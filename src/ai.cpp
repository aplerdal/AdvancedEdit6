#include "ai.hpp"

#include "editor.hpp"
#include <limits>
#include <cmath>

#include "tileset.hpp"

std::string AI::get_name(){
    return "AI";
}
AI::AI(){
    view = new ViewTool();
}
void AI::update(AppState* as){
    if (!open) return;
    ImGui::Begin("AI Editor", &open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (!as->editor_ctx.file_open || as->editor_ctx.selected_track < 0) {
        ImGui::Text("No Track Loaded");
        ImGui::End();
        return;
    }
    if (as->editor_ctx.map_buffer == nullptr) {
        ImGui::TextColored(ImVec4(1.0f,0.25f,0.25f,1.0f), "Error Reading Map Image!");
        ImGui::End();
        return;
    }
    if (as->game_ctx.tracks[as->editor_ctx.selected_track].ai_header == nullptr){
        ImGui::TextColored(ImVec4(1.0f,0.25f,0.25f,1.0f), "Error Reading AI data!");
        ImGui::End();
        return;
    }
    // AI Menu Bar
    if (ImGui::BeginMenuBar()){
        if (ImGui::BeginMenu("View")){
            if (ImGui::MenuItem("Reset View")){
                state.translation = ImVec2();
                state.scale = 1.0f;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")){
            if (ImGui::MenuItem("Undo", "ctrl+z")){
                undo(as);
            }
            if (ImGui::MenuItem("Redo", "ctrl+y")){
                redo(as);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Sector")) {
            if (ImGui::MenuItem("Create Sector")) {
                CreateSector(as);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    // Draw Track Image
    state.win_pos = ImGui::GetWindowPos();
    state.win_size = ImGui::GetWindowSize();
    state.cursor_pos = ImVec2(state.win_pos.x + state.translation.x, state.win_pos.y + state.translation.y);
    state.track_size = ImVec2((as->game_ctx.track_width*TILE_SIZE*state.scale), (as->game_ctx.track_height*TILE_SIZE*state.scale));
    
    ImGui::SetCursorScreenPos(state.cursor_pos);
    ImGui::Image((ImTextureID)(intptr_t)as->editor_ctx.map_buffer, ImVec2(state.track_size.x,state.track_size.y));

    DrawLayout(as); 
    if (ImGui::IsWindowFocused()){
        if (state.scale < 1.0f) 
            SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_LINEAR);
        else
            SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_NEAREST);
        // Handle Tools
        view->update(as, state);
        
        // Claim inspector
        as->editor_ctx.inspector = this;

        HandleInput(as, &as->game_ctx.tracks[as->editor_ctx.selected_track]);
        
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) {
            undo(as);
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y)) {
            redo(as);
        }
    }
    
    ImGui::End();
}
void AI::inspector(AppState* as) {
    ImGui::Text("AI Inspector");
    ImGui::Separator();
    if (ImGui::Button("Create Sector")) {
        CreateSector(as);
    }
    ImGui::Separator();
    if (selected_sector > -1 && selected_part != SECTOR_PART_NONE) {
        TrackContext* t = &as->game_ctx.tracks[as->editor_ctx.selected_track];
        if (selected_part == SECTOR_PART_ZONE) {
            auto zone = t->ai_zones[selected_sector];
            ImGui::Text("Zone %d:", selected_sector);
            int ishape = zone->shape;
            ImGui::Combo("Shape", &ishape, "Rectangle\0Triangle Top Left\0Triangle Top Right\0Triangle Bottom Right\0Triangle Bottom Left\0");
            zone->shape = (uint8_t)ishape;
        } else if (selected_part == SECTOR_PART_TARGET) {
            auto target = t->ai_targets[0][selected_sector];
            ImGui::Text("Target %d:", selected_sector);
            int ispeed = target->speed & TARGET_MASK_SPEED;
            ImGui::InputInt("Speed", &ispeed);
            target->speed = (target->speed & TARGET_MASK_FLAGS) | ((uint8_t)ispeed&TARGET_MASK_SPEED);
            uint32_t uflags = (uint32_t)target->flags&TARGET_MASK_FLAGS;
            ImGui::CheckboxFlags("Intersection?", &uflags, TARGET_FLAGS_INTERSECTION);
            target->flags = ((uint8_t)(uflags&TARGET_MASK_FLAGS)) | (target->flags&TARGET_MASK_SPEED);
        } else {
            ImGui::Text("You shouldn't be seeing this...");
        }
    }
}
void AI::DrawLayout(AppState *as){
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
	ImVec2 vMax = ImGui::GetWindowContentRegionMax();

    vMin.x += ImGui::GetWindowPos().x - ImGui::GetStyle().WindowPadding.x;
    vMin.y += ImGui::GetWindowPos().y - ImGui::GetStyle().WindowPadding.y;
    vMax.x += ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x;
    vMax.y += ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y;
    
    dl->PushClipRect(vMin, vMax);

    TrackContext* t = &as->game_ctx.tracks[as->editor_ctx.selected_track];
    AI::SectorDraw(dl, t);

    dl->PopClipRect();
}
static void SetHover(SectorPart part, SectorPart& current, int new_hov_sec, int& hov_sec) {
    if ((int)part>(int)current) {
        current = part;
        hov_sec = new_hov_sec;
    }
}
void AI::HandleInput(AppState*as, TrackContext* t) {
    ImVec2 mouse_pos = ImGui::GetMousePos();
    hovered_sector = -1;
    hover_part = SECTOR_PART_NONE;
    // Get hovered sector and part
    for (int i = 0; i<t->ai_header->count; i++) {
        auto zone = t->ai_zones[i];
        auto target = t->ai_targets[0][i];

        float sel_dist = SEL_DIST * state.scale;
        if (zone->shape == ZONE_SHAPE_RECTANGLE) {
            ImVec2 min = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            ImVec2 max = ImVec2(min.x + (state.scale*(zone->half_width+1)*TILE_SIZE*2.0f), min.y + (state.scale*(zone->half_height+1)*TILE_SIZE*2.0f));
            ImVec2 top_right = ImVec2(max.x,min.y);
            ImVec2 bot_left = ImVec2(min.x, max.y);
            
            SectorPart hov_part;
            if (PointInCircle(mouse_pos,min,sel_dist)) {
                hov_part = SECTOR_PART_SCALE_NW;
            } else if (PointInCircle(mouse_pos,max,sel_dist)) {
                hov_part = SECTOR_PART_SCALE_SE;
            } else if (PointInCircle(mouse_pos,top_right,sel_dist)) {
                hov_part = SECTOR_PART_SCALE_NE;
            } else if (PointInCircle(mouse_pos,bot_left,sel_dist)) {
                hov_part = SECTOR_PART_SCALE_SW;
            } else if (PointInRect(mouse_pos,ImVec2(min.x,min.y-sel_dist),ImVec2(max.x,min.y+sel_dist))) {
                hov_part = SECTOR_PART_SCALE_N;
            } else if (PointInRect(mouse_pos,ImVec2(min.x,max.y-sel_dist),ImVec2(max.x,max.y+sel_dist))) {
                hov_part = SECTOR_PART_SCALE_S;
            } else if (PointInRect(mouse_pos,ImVec2(min.x-sel_dist, min.y),ImVec2(min.x+sel_dist, max.y))) {
                hov_part = SECTOR_PART_SCALE_E;
            } else if (PointInRect(mouse_pos,ImVec2(max.x-sel_dist, min.y),ImVec2(max.x+sel_dist, max.y))){
                hov_part = SECTOR_PART_SCALE_W;
            } else if (PointInRect(mouse_pos, min, max)) {
                hov_part = SECTOR_PART_ZONE;
            } else {
                // Lowest priority, as to not override existing hovered part;
                hov_part = SECTOR_PART_NONE;
            }
            SetHover(hov_part, hover_part, i, hovered_sector);
        } else {
            ImVec2 vertex, armx, army;
            GetZonePoints(zone, vertex, armx, army);
            vertex = state.cursor_pos + (vertex*state.scale*TILE_SIZE*2.0f);
            armx = state.cursor_pos + (armx*state.scale*TILE_SIZE*2.0f);
            army = state.cursor_pos + (army*state.scale*TILE_SIZE*2.0f);
            
            SectorPart hov_part;

            ImVec2 offsetx = ImVec2((armx.x<vertex.x)?-sel_dist:sel_dist,0);
            ImVec2 offsety = ImVec2(0,(army.y<vertex.y)?-sel_dist:sel_dist);


            if (PointInTriangle(mouse_pos, vertex, armx-offsetx, army-offsety) != PointInTriangle(mouse_pos, vertex, armx+offsetx, army+offsety)) {
                hov_part = SECTOR_PART_SCALE_HYPOT;
            } else if (false) {
                hov_part = SECTOR_PART_SCALE_ANGLE;
            } else if (PointInTriangle(mouse_pos, vertex, armx, army)) {
                hov_part = SECTOR_PART_ZONE;
            } else {
                hov_part = SECTOR_PART_NONE;
            }
            SetHover(hov_part, hover_part, i, hovered_sector);

        }

        ImVec2 target_pos = state.cursor_pos + (ImVec2(target->x, target->y) * state.scale * TILE_SIZE);

        if (PointInCircle(mouse_pos,target_pos,sel_dist)) {
            SetHover(SECTOR_PART_TARGET, hover_part, i, hovered_sector);
        }
    }
    
    if (dragging) {
        // Handle input when dragging
        auto zone = t->ai_zones[drag_sector];
        auto target = t->ai_targets[0][drag_sector];
        hovered_sector = drag_sector;
        hover_part = drag_part;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImVec2 mouse_rel = ImVec2(
                (mouse_pos.x - state.cursor_pos.x)/(state.scale * TILE_SIZE),
                (mouse_pos.y - state.cursor_pos.y)/(state.scale * TILE_SIZE)
            );
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            uint16_t delta;
            switch (drag_part){
                case SECTOR_PART_TARGET:
                    target->x = clampCast<uint16_t>(mouse_rel.x);
                    target->y = clampCast<uint16_t>(mouse_rel.y);
                    break;
                case SECTOR_PART_ZONE:
                    zone->half_x = clampCast<uint16_t>((mouse_rel.x / 2.0f) - drag_offset.x);
                    zone->half_y = clampCast<uint16_t>((mouse_rel.y / 2.0f) - drag_offset.y);
                    break;
                case SECTOR_PART_SCALE_NW:
                    delta = zone->half_x - clampCast<uint16_t>(mouse_rel.x / 2.0f);
                    zone->half_x -= delta;
                    zone->half_width += delta;

                    delta = zone->half_y - clampCast<uint16_t>(mouse_rel.y / 2.0f);
                    zone->half_y -= delta;
                    zone->half_height += delta;
                    break;
                case SECTOR_PART_SCALE_NE:
                    zone->half_width = clampCast<uint16_t>((mouse_rel.x / 2.0f) - zone->half_x) - 1;

                    delta = zone->half_y - clampCast<uint16_t>(mouse_rel.y / 2.0f);
                    zone->half_y -= delta;
                    zone->half_height += delta;
                    break;
                case SECTOR_PART_SCALE_SW:
                    delta = zone->half_x - clampCast<uint16_t>(mouse_rel.x / 2.0f);
                    zone->half_x -= delta;
                    zone->half_width += delta;

                    zone->half_height = clampCast<uint16_t>((mouse_rel.y / 2.0f)-zone->half_y) - 1;
                    break;
                case SECTOR_PART_SCALE_SE:
                    zone->half_width = clampCast<uint16_t>((mouse_rel.x / 2.0f) - zone->half_x) - 1;

                    zone->half_height = clampCast<uint16_t>((mouse_rel.y / 2.0f)-zone->half_y) - 1;
                    break;
                case SECTOR_PART_SCALE_E:
                    delta = zone->half_x - clampCast<uint16_t>(mouse_rel.x / 2.0f);
                    zone->half_x -= delta;
                    zone->half_width += delta;
                    break;
                case SECTOR_PART_SCALE_W:
                    zone->half_width = clampCast<uint16_t>((mouse_rel.x / 2.0f) - zone->half_x) - 1;
                    break;
                case SECTOR_PART_SCALE_S:
                    zone->half_height = clampCast<uint16_t>((mouse_rel.y / 2.0f)-zone->half_y) - 1;
                    break;
                case SECTOR_PART_SCALE_N:
                    delta = zone->half_y - clampCast<uint16_t>(mouse_rel.y / 2.0f);
                    zone->half_y -= delta;
                    zone->half_height += delta;
                    break;
                case SECTOR_PART_SCALE_HYPOT:
                    zone->half_width = clampCast<uint16_t>(abs((mouse_rel.x / 2.0f) - zone->half_x)); // TODO Correct triangle type offsets
                    break;
            }   
        } else {
            // dragging, but not holding mouse down. Handle end of drag
            PUSH_STACK(
                as->editor_ctx.undo_stack,
                new AiModifyCmd(as, drag_sector, old_zone, *zone, old_target, *target)
            );
            dragging = false;

            if (hover_part == SECTOR_PART_TARGET || hover_part == SECTOR_PART_ZONE) {
                selected_part = hover_part;
                selected_sector = hovered_sector;
            }
        }
    } else {
        // Handle input on sector if not dragging
        if (hovered_sector > -1 && hover_part != SECTOR_PART_NONE) {
            auto zone = t->ai_zones[hovered_sector];
            auto target = t->ai_targets[0][hovered_sector];
            switch (hover_part) {
                case SECTOR_PART_ZONE:
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                        BeginDrag(hovered_sector,SECTOR_PART_ZONE, *zone, *target, (mouse_pos-state.cursor_pos)/(state.scale*TILE_SIZE*2.0f)-ImVec2(zone->half_x, zone->half_y));
                    } break;
                case SECTOR_PART_SCALE_HYPOT:
                    switch (zone->shape)
                    {
                        case ZONE_SHAPE_TRIANGLE_BOTTOM_LEFT:
                        case ZONE_SHAPE_TRIANGLE_TOP_RIGHT:
                            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
                            break;
                        case ZONE_SHAPE_TRIANGLE_TOP_LEFT:
                        case ZONE_SHAPE_TRIANGLE_BOTTOM_RIGHT:
                            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                            break;
                    }
                    BeginDrag(hovered_sector,SECTOR_PART_SCALE_HYPOT, *zone, *target, (mouse_pos-state.cursor_pos)/(state.scale*TILE_SIZE*2.0f)-ImVec2(zone->half_x, zone->half_y));
                    break;
                case SECTOR_PART_SCALE_E:
                case SECTOR_PART_SCALE_W:
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                    goto scaleDrag;
                case SECTOR_PART_SCALE_N:
                case SECTOR_PART_SCALE_S:
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                    goto scaleDrag;
                case SECTOR_PART_SCALE_NE:
                case SECTOR_PART_SCALE_SW:
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
                    goto scaleDrag;
                case SECTOR_PART_SCALE_NW:
                case SECTOR_PART_SCALE_SE:
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                scaleDrag:
                    BeginDrag(hovered_sector, hover_part, *zone, *target, ImVec2());
                    break;
                case SECTOR_PART_TARGET:
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    BeginDrag(hovered_sector, hover_part, *zone, *target, ImVec2());
                    break;
            }
        }
        
        // Reset selected zone when releasing mouse and not dragging
        if (selected_sector > -1 && selected_part != SECTOR_PART_NONE && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            selected_sector = -1;
            selected_part = SECTOR_PART_NONE;
        }
    }
}
const ImColor fill_color = ImColor(0.8f,0.1f,0.7f, 0.3f);
const ImColor hover_color = ImColor(0.8f,0.1f,0.7f, 0.5f);
const ImColor border_color = ImColor(0.8f,0.1f,0.7f, 1.0f);
void AI::SectorDraw(ImDrawList* dl, TrackContext* t) {
    for (int i = 0; i < t->ai_header->count; i++) {
        auto zone = t->ai_zones[i];
        auto target = t->ai_targets[0][i];
        bool zone_hovered = (i==hovered_sector && hover_part == SECTOR_PART_ZONE)||(i==selected_sector && selected_part == SECTOR_PART_ZONE);
        bool target_hovered = (i==hovered_sector && hover_part == SECTOR_PART_TARGET)||(i==selected_sector && selected_part == SECTOR_PART_TARGET);
        
        float border_scale = zone_hovered? HOVER_BORDER_SIZE:1.0f;
        float sel_dist = SEL_DIST * state.scale;

        if (zone->shape == ZONE_SHAPE_RECTANGLE) {
            // Zone
            auto min = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            auto max = ImVec2(min.x + (state.scale*(zone->half_width+1)*TILE_SIZE*2.0f), min.y + (state.scale*(zone->half_height+1)*TILE_SIZE*2.0f));

            dl->AddRectFilled(min, max, (zone_hovered||target_hovered)?hover_color:fill_color);
            dl->AddRect(min,max,border_color, 0.0f, 0, border_scale);

            // Target
            ImVec2 rmin = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            ImVec2 rmax = ImVec2(rmin.x + (state.scale*(zone->half_width+1)*TILE_SIZE*2.0f), rmin.y + (state.scale*(zone->half_height+1)*TILE_SIZE*2.0f));
            ImVec2 target_pos = ImVec2(state.cursor_pos.x+(state.scale*target->x*TILE_SIZE), state.cursor_pos.y+(state.scale*target->y*TILE_SIZE));
            
            dl->AddCircleFilled(target_pos, sel_dist, target_hovered?hover_color:fill_color);
            dl->AddCircle(target_pos, sel_dist, border_color, 0, target_hovered? HOVER_BORDER_SIZE:1.0f);

            if (target_pos.x>=rmin.x && target_pos.x<=rmax.x) {
                float dist_min = std::abs(target_pos.y-rmin.y);
                float dist_max = std::abs(target_pos.y-rmax.y);
                
                if (dist_min > dist_max) {
                    dl->AddLine(target_pos, rmax, border_color, border_scale);
                    dl->AddLine(target_pos, ImVec2(rmin.x, rmax.y), border_color, border_scale);
                } else {
                    dl->AddLine(target_pos, rmin, border_color, border_scale);
                    dl->AddLine(target_pos, ImVec2(rmax.x, rmin.y), border_color, border_scale);
                }
            } else if (target_pos.y>=rmin.y && target_pos.y<=rmax.y) {
                float dist_min = std::abs(target_pos.x-rmin.x);
                float dist_max = std::abs(target_pos.x-rmax.x);

                if (dist_min > dist_max) {
                    dl->AddLine(target_pos, rmax, border_color, border_scale);
                    dl->AddLine(target_pos, ImVec2(rmax.x, rmin.y), border_color, border_scale);
                } else {
                    dl->AddLine(target_pos, rmin, border_color, border_scale);
                    dl->AddLine(target_pos, ImVec2(rmin.x, rmax.y), border_color, border_scale);
                }
            } else {
                if ((target_pos.x <= rmin.x && target_pos.y <= rmin.y) || (target_pos.x >= rmax.x && target_pos.y >= rmax.y)) {
                    dl->AddLine(target_pos, ImVec2(rmax.x, rmin.y), border_color, border_scale);
                    dl->AddLine(target_pos, ImVec2(rmin.x, rmax.y), border_color, border_scale);
                } else {
                    dl->AddLine(target_pos, rmin, border_color, border_scale);
                    dl->AddLine(target_pos, rmax, border_color, border_scale);
                }
            }
        } else {
            // Triangle Zone
            ImVec2 vertex, armx, army;
            GetZonePoints(zone, vertex, armx, army);
            
            vertex = state.cursor_pos + vertex * state.scale * TILE_SIZE * 2.0f;
            armx = state.cursor_pos + armx * state.scale * TILE_SIZE * 2.0f;
            army = state.cursor_pos + army * state.scale * TILE_SIZE * 2.0f;
            dl->AddTriangleFilled(vertex, armx, army, (zone_hovered||target_hovered)?hover_color : fill_color);
            dl->AddTriangle(vertex, armx, army, border_color, border_scale);
            
            // Target
            ImVec2 target_pos = ImVec2(state.cursor_pos.x+(state.scale*target->x*TILE_SIZE), state.cursor_pos.y+(state.scale*target->y*TILE_SIZE));

            float armx_dist = std::hypot(armx.x - target_pos.x, armx.y - target_pos.y);
            float army_dist = std::hypot(army.x - target_pos.x, army.y - target_pos.y);
            float vert_dist = std::hypot(vertex.x - target_pos.x, vertex.y - target_pos.y);

            dl->AddCircleFilled(target_pos, sel_dist, target_hovered?hover_color:fill_color);
            dl->AddCircle(target_pos, sel_dist, border_color, 0, zone_hovered? HOVER_BORDER_SIZE:1.0f);

            if (armx_dist > army_dist && armx_dist > vert_dist) {
                dl->AddLine(target_pos, vertex, border_color, border_scale);
                dl->AddLine(target_pos, army, border_color, border_scale);
            } else if (army_dist > armx_dist && army_dist > vert_dist) {
                dl->AddLine(target_pos, vertex, border_color, border_scale);
                dl->AddLine(target_pos, armx, border_color, border_scale);
            } else {
                dl->AddLine(target_pos, armx, border_color, border_scale);
                dl->AddLine(target_pos, army, border_color, border_scale);
            }
        }
    }
}
void AI::CreateSector(AppState* as) {
    AiZone* ex_zone = new AiZone();
    ex_zone->half_width = 8;
    ex_zone->half_height = 8;
    ex_zone->shape = ZONE_SHAPE_RECTANGLE;
    AiTarget* ex_target = new AiTarget();
    ex_target->x = 8;
    ex_target->y = 8;
    ex_target->flags = 2;
    PUSH_STACK(as->editor_ctx.undo_stack, new CreateSectorCmd(as, ex_zone, ex_target));
}

void AI::BeginDrag(int sector, SectorPart part, AiZone old_zone, AiTarget old_target, ImVec2 offset){
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (!dragging) {
            dragging = true;
            drag_sector = sector;
            drag_part = part;
            this->old_zone = old_zone;
            this->old_target = old_target;
            drag_offset = offset;
        }
    }
}

static void GetZonePoints(AiZone* zone, ImVec2& vertex, ImVec2& armx, ImVec2& army) {
    switch (zone->shape)
    {
    #pragma warning(disable : 244)
        case ZONE_SHAPE_TRIANGLE_BOTTOM_RIGHT:
            vertex = ImVec2(zone->half_x+1, zone->half_y+1);
            armx = ImVec2(vertex.x-(zone->half_width+1),vertex.y);
            army = ImVec2(vertex.x,vertex.y-(zone->half_width+1));
            break;
        case ZONE_SHAPE_TRIANGLE_TOP_LEFT:
            vertex = ImVec2(zone->half_x, zone->half_y);
            armx = ImVec2(vertex.x+zone->half_width+1,vertex.y);
            army = ImVec2(vertex.x,vertex.y+zone->half_width+1);
            break;
        case ZONE_SHAPE_TRIANGLE_TOP_RIGHT:
            vertex = ImVec2(zone->half_x+1, zone->half_y);
            armx = ImVec2(vertex.x-(zone->half_width+1),vertex.y);
            army = ImVec2(vertex.x,vertex.y+zone->half_width+1);
            break;
        case ZONE_SHAPE_TRIANGLE_BOTTOM_LEFT:
            vertex = ImVec2(zone->half_x, zone->half_y+1);
            armx = ImVec2(vertex.x+zone->half_width+1,vertex.y);
            army = ImVec2(vertex.x,vertex.y-(zone->half_width+1));
            break;
    }
    #pragma warning(default : 244)
}

static bool PointAboveLine(ImVec2 point, ImVec2 p1, ImVec2 p2){
    return (point.x - p1.x) * (p2.y - p1.y) - (point.y - p1.y) * (p2.x - p1.x) > 0;
}
static bool PointInTriangle(ImVec2 point, ImVec2 vertex, ImVec2 armx, ImVec2 army){
    ImVec2 min = ImVec2(__min(__min(armx.x, army.x),vertex.x),__min(__min(armx.y, army.y),vertex.y));
    ImVec2 max = ImVec2(__max(__max(armx.x, army.x),vertex.x),__max(__max(armx.y, army.y),vertex.y));
    if (PointInRect(point, min, max)){
        return PointAboveLine(vertex, armx, army)==PointAboveLine(point, armx, army);
    }
    return false;
}
static bool PointInCircle(ImVec2 point, ImVec2 position, float radius) {
    return std::hypot(point.x-position.x, point.y-position.y) <= radius;
}
static bool PointInRect(ImVec2 point, ImVec2 min, ImVec2 max){
    return (point.x >= min.x && point.y >= min.y) && (point.x <= max.x && point.y <= max.y);
}

template<typename T>
T clampCast(float value) {
    return (T)SDL_clamp(value, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
}

void AI::undo(AppState *as)
{
    if (as->editor_ctx.undo_stack.size() > 0) {
        as->editor_ctx.undo_stack.back()->undo(as);
        as->editor_ctx.redo_stack.push_back(as->editor_ctx.undo_stack.back());
        as->editor_ctx.undo_stack.pop_back();
    }
    if (as->editor_ctx.undo_stack.size() > 32) {
        as->editor_ctx.undo_stack.pop_front();
    }
}
void AI::redo(AppState *as)
{
    if (as->editor_ctx.redo_stack.size() > 0) {
        as->editor_ctx.redo_stack.back()->redo(as);
        as->editor_ctx.undo_stack.push_back(as->editor_ctx.redo_stack.back());
        as->editor_ctx.redo_stack.pop_back();
    }
    if (as->editor_ctx.redo_stack.size() > 32) {
        as->editor_ctx.redo_stack.pop_front();
    }
}

AiModifyCmd::AiModifyCmd(AppState* as, int drag_sector, AiZone old_zone, AiZone new_zone, AiTarget old_target, AiTarget new_target){
    this->old_zone = old_zone;
    this->new_zone = new_zone;
    this->old_target = old_target;
    this->new_target = new_target;
    this->drag_sector = drag_sector;
}
void AiModifyCmd::redo(AppState *as)
{
    auto zone = as->game_ctx.tracks[as->editor_ctx.selected_track].ai_zones[drag_sector];
    auto target = as->game_ctx.tracks[as->editor_ctx.selected_track].ai_targets[0][drag_sector];
    
    zone->half_x = new_zone.half_x;
    zone->half_y = new_zone.half_y;
    zone->half_width = new_zone.half_width;
    zone->half_height = new_zone.half_height;

    target->x = new_target.x;
    target->x = new_target.y;
    target->flags = new_target.flags;
    target->speed = new_target.speed;
}
void AiModifyCmd::undo(AppState *as)
{
    
    auto zone = as->game_ctx.tracks[as->editor_ctx.selected_track].ai_zones[drag_sector];
    auto target = as->game_ctx.tracks[as->editor_ctx.selected_track].ai_targets[0][drag_sector];

    zone->half_x = old_zone.half_x;
    zone->half_y = old_zone.half_y;
    zone->half_width = old_zone.half_width;
    zone->half_height = old_zone.half_height;

    target->x = old_target.x;
    target->y = old_target.y;
    target->flags = old_target.flags;
    target->speed = old_target.speed;
}
void AiModifyCmd::execute(AppState *as) {

}

CreateSectorCmd::CreateSectorCmd(AppState* as, AiZone* new_zone, AiTarget* new_target) {
    this->new_target = new_target;
    this->new_zone = new_zone;
    redo(as);
}
void CreateSectorCmd::execute(AppState* as){
}
void CreateSectorCmd::undo(AppState* as) {
    auto track = &as->game_ctx.tracks[as->editor_ctx.selected_track];
    track->ai_header->count -= 1;
    track->ai_zones.pop_back();
}
void CreateSectorCmd::redo(AppState* as) {
    auto track = &as->game_ctx.tracks[as->editor_ctx.selected_track];
    track->ai_targets[0].push_back(new_target);
    track->ai_targets[1].push_back(new_target);
    track->ai_targets[2].push_back(new_target);
    track->ai_zones.push_back(new_zone);
    track->ai_header->count += 1;
    printf("- Appended sector -\n");

}