#include "ai.hpp"

#include <SDL3/SDL.h>
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

        float sel_circle_rad = CIRCLE_RAD * state.scale;
        if (zone->shape == ZONE_SHAPE_RECTANGLE) {
            ImVec2 min = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            ImVec2 max = ImVec2(min.x + (state.scale*(zone->half_width+1)*TILE_SIZE*2.0f), min.y + (state.scale*(zone->half_height+1)*TILE_SIZE*2.0f));
            ImVec2 top_right = ImVec2(max.x,min.y);
            ImVec2 bot_left = ImVec2(min.x, max.y);
            
            SectorPart hov_part;
            if (PointInCircle(mouse_pos,min,sel_circle_rad)) {
                hov_part = SECTOR_PART_SCALE_NW;
            } else if (PointInCircle(mouse_pos,max,sel_circle_rad)) {
                hov_part = SECTOR_PART_SCALE_SE;
            } else if (PointInCircle(mouse_pos,top_right,sel_circle_rad)) {
                hov_part = SECTOR_PART_SCALE_NE;
            } else if (PointInCircle(mouse_pos,bot_left,sel_circle_rad)) {
                hov_part = SECTOR_PART_SCALE_SW;
            } else if (PointInRect(mouse_pos,ImVec2(min.x,min.y-sel_circle_rad),ImVec2(max.x,min.y+sel_circle_rad))) {
                hov_part = SECTOR_PART_SCALE_N;
            } else if (PointInRect(mouse_pos,ImVec2(min.x,max.y-sel_circle_rad),ImVec2(max.x,max.y+sel_circle_rad))) {
                hov_part = SECTOR_PART_SCALE_S;
            } else if (PointInRect(mouse_pos,ImVec2(min.x-sel_circle_rad, min.y),ImVec2(min.x+sel_circle_rad, max.y))) {
                hov_part = SECTOR_PART_SCALE_E;
            } else if (PointInRect(mouse_pos,ImVec2(max.x-sel_circle_rad, min.y),ImVec2(max.x+sel_circle_rad, max.y))){
                hov_part = SECTOR_PART_SCALE_W;
            } else if (PointInRect(mouse_pos, min, max)) {
                hov_part = SECTOR_PART_ZONE;
            } else {
                // Lowest priority, will not override existing stuff
                hov_part = SECTOR_PART_NONE;
            }
            SetHover(hov_part, hover_part, i, hovered_sector);
        } else {
            ImVec2 vertex, armx, army;
            GetZonePoints(zone, vertex, armx, army);
            vertex = state.cursor_pos + (vertex*state.scale*TILE_SIZE*2.0f);
            armx = state.cursor_pos + (armx*state.scale*TILE_SIZE*2.0f);
            army = state.cursor_pos + (army*state.scale*TILE_SIZE*2.0f);
            // ERR
            if (PointInTriangle(mouse_pos, vertex, armx, army)) {
                SetHover(SECTOR_PART_ZONE, hover_part, i, hovered_sector);
            }
        }

        ImVec2 target_pos = state.cursor_pos + (ImVec2(target->x, target->y) * state.scale * TILE_SIZE);

        if (PointInCircle(mouse_pos,target_pos,sel_circle_rad)) {
            SetHover(SECTOR_PART_TARGET, hover_part, i, hovered_sector);
        }
    }
    // Handle input on sector if not dragging
    if (dragging) {
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
                    zone->half_width += delta-1;

                    delta = zone->half_y - clampCast<uint16_t>(mouse_rel.y / 2.0f);
                    zone->half_y -= delta;
                    zone->half_height += delta-1;
                    break;
                case SECTOR_PART_SCALE_NE:
                    zone->half_width = clampCast<uint16_t>((mouse_rel.x / 2.0f) - zone->half_x) - 1;

                    delta = zone->half_y - clampCast<uint16_t>(mouse_rel.y / 2.0f);
                    zone->half_y -= delta;
                    zone->half_height += delta-1;
                    break;
                case SECTOR_PART_SCALE_SW:
                    delta = zone->half_x - clampCast<uint16_t>(mouse_rel.x / 2.0f);
                    zone->half_x -= delta;
                    zone->half_width += delta - 1;

                    zone->half_height = clampCast<uint16_t>((mouse_rel.y / 2.0f)-zone->half_y) - 1;
                    break;
                case SECTOR_PART_SCALE_SE:
                    zone->half_width = clampCast<uint16_t>((mouse_rel.x / 2.0f) - zone->half_x) - 1;

                    zone->half_height = clampCast<uint16_t>((mouse_rel.y / 2.0f)-zone->half_y) - 1;
                    break;
                case SECTOR_PART_SCALE_E:
                    delta = zone->half_x - clampCast<uint16_t>(mouse_rel.x / 2.0f);
                    zone->half_x -= delta;
                    zone->half_width += delta - 1;
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
                    zone->half_height += delta - 1;
                    break;
            }   
        } else {
            // dragging, but not holding mouse donw. Handle end of drag
            PUSH_STACK(
                as->editor_ctx.undo_stack,
                new AiModifyCmd(as, drag_sector, old_zone, *zone, old_target, *target)
            );
            dragging = false;
        }
    } else if (hovered_sector > -1 && hover_part != SECTOR_PART_NONE) {
        auto zone = t->ai_zones[hovered_sector];
        auto target = t->ai_targets[0][hovered_sector];
        switch (hover_part) {
            case SECTOR_PART_ZONE:
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    BeginDrag(hovered_sector,SECTOR_PART_ZONE, *zone, *target,ImVec2(
                            ((mouse_pos.x - state.cursor_pos.x)/(state.scale * TILE_SIZE * 2.0f))-zone->half_x,
                            ((mouse_pos.y - state.cursor_pos.y)/(state.scale * TILE_SIZE * 2.0f))-zone->half_y
                        )
                    );
                } break;
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
}
const ImColor fill_color = ImColor(0.8f,0.1f,0.7f, 0.3f);
const ImColor hover_color = ImColor(0.8f,0.1f,0.7f, 0.5f);
const ImColor border_color = ImColor(0.8f,0.1f,0.7f, 1.0f);
void AI::SectorDraw(ImDrawList* dl, TrackContext* t) {
    for (int i = 0; i < t->ai_header->count; i++) {
        auto zone = t->ai_zones[i];
        auto target = t->ai_targets[0][i];

        bool zone_hovered = (i==hovered_sector && hover_part==SECTOR_PART_ZONE);
        bool target_hovered = (i==hovered_sector && hover_part == SECTOR_PART_TARGET);
        
        float border_scale = zone_hovered? HOVER_BORDER_SIZE:1.0f;
        float sel_circle_rad = CIRCLE_RAD * state.scale;

        if (zone->shape == ZONE_SHAPE_RECTANGLE) {
            // Zone
            auto min = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            auto max = ImVec2(min.x + (state.scale*(zone->half_width+1)*TILE_SIZE*2.0f), min.y + (state.scale*(zone->half_height+1)*TILE_SIZE*2.0f));

            dl->AddRectFilled(min, max, zone_hovered?hover_color:fill_color);
            dl->AddRect(min,max,border_color, 0.0f, 0, border_scale);

            // Target
            ImVec2 rmin = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            ImVec2 rmax = ImVec2(rmin.x + (state.scale*(zone->half_width+1)*TILE_SIZE*2.0f), rmin.y + (state.scale*(zone->half_height+1)*TILE_SIZE*2.0f));
            ImVec2 target_pos = ImVec2(state.cursor_pos.x+(state.scale*target->x*TILE_SIZE), state.cursor_pos.y+(state.scale*target->y*TILE_SIZE));
            
            dl->AddCircleFilled(target_pos, sel_circle_rad, target_hovered?hover_color:fill_color);
            dl->AddCircle(target_pos, sel_circle_rad, border_color, 0, target_hovered? HOVER_BORDER_SIZE:1.0f);

            if (target_pos.x>=rmin.x && target_pos.x<=rmax.x) {
                float dist_min = std::abs(target_pos.y-rmin.y);
                float dist_max = std::abs(target_pos.y-rmax.y);
                
                if (dist_min > dist_max)
                    dl->AddTriangle(target_pos, rmax, ImVec2(rmin.x, rmax.y), border_color, border_scale);
                else
                    dl->AddTriangle(target_pos, rmin, ImVec2(rmax.x, rmin.y), border_color, border_scale);
            } else if (target_pos.y>=rmin.y && target_pos.y<=rmax.y) {
                float dist_min = std::abs(target_pos.x-rmin.x);
                float dist_max = std::abs(target_pos.x-rmax.x);

                if (dist_min > dist_max)
                    dl->AddTriangle(target_pos, rmax, ImVec2(rmax.x, rmin.y), border_color, border_scale);
                else
                    dl->AddTriangle(target_pos, rmin, ImVec2(rmin.x, rmax.y), border_color, border_scale);
            } else {
                if ((target_pos.x <= rmin.x && target_pos.y <= rmin.y) || (target_pos.x >= rmax.x && target_pos.y >= rmax.y))
                    dl->AddTriangle(target_pos, ImVec2(rmax.x,rmin.y), ImVec2(rmin.x, rmax.y), border_color, border_scale);
                else
                    dl->AddTriangle(target_pos, rmin, rmax, border_color, border_scale);
            }
        } else {
            // Triangle Zone
            //ImVec2 vertex = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            //float tri_size = state.scale*(zone->half_width+1)*TILE_SIZE*2.0f;
            ImVec2 vertex, armx, army;
            GetZonePoints(zone, vertex, armx, army);
            // Holy shit I need to make my own vector type or use a different one this sucks ass
            vertex = ImVec2(state.cursor_pos.x + vertex.x * state.scale * TILE_SIZE * 2.0f, state.cursor_pos.y + vertex.y * state.scale * TILE_SIZE * 2.0f);
            armx = ImVec2(state.cursor_pos.x + armx.x * state.scale * TILE_SIZE * 2.0f, state.cursor_pos.y + armx.y * state.scale * TILE_SIZE * 2.0f);
            army = ImVec2(state.cursor_pos.x + army.x * state.scale * TILE_SIZE * 2.0f, state.cursor_pos.y + army.y * state.scale * TILE_SIZE * 2.0f);

            dl->AddTriangleFilled(vertex, armx, army, zone_hovered?hover_color : fill_color);
            dl->AddTriangle(vertex, armx, army, border_color, border_scale);

            // Target
            ImVec2 target_pos = ImVec2(state.cursor_pos.x+(state.scale*target->x*TILE_SIZE), state.cursor_pos.y+(state.scale*target->y*TILE_SIZE));

            float armx_dist = std::hypot(armx.x - target_pos.x, armx.y - target_pos.y);
            float army_dist = std::hypot(army.x - target_pos.x, army.y - target_pos.y);
            float vert_dist = std::hypot(vertex.x - target_pos.x, vertex.y - target_pos.y);

            dl->AddCircleFilled(target_pos, sel_circle_rad, target_hovered?hover_color:fill_color);
            dl->AddCircle(target_pos, sel_circle_rad, target_hovered, 0, zone_hovered? HOVER_BORDER_SIZE:1.0f);

            if (armx_dist > army_dist && armx_dist > vert_dist)
                dl->AddTriangle(vertex, army, target_pos, border_color, border_scale);
            else if (army_dist > armx_dist && army_dist > vert_dist)
                dl->AddTriangle(vertex, armx, target_pos, border_color, border_scale);
            else
                dl->AddTriangle(armx, army, target_pos, border_color, border_scale);
        }
    }
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
            armx = ImVec2(vertex.x,vertex.y-(zone->half_width+1));
            army = ImVec2(vertex.x-(zone->half_width+1),vertex.y);
            break;
        case ZONE_SHAPE_TRIANGLE_TOP_LEFT:
            vertex = ImVec2(zone->half_x, zone->half_y);
            armx = ImVec2(vertex.x,vertex.y+zone->half_width+1);
            army = ImVec2(vertex.x+zone->half_width+1,vertex.y);
            break;
        case ZONE_SHAPE_TRIANGLE_TOP_RIGHT:
            vertex = ImVec2(zone->half_x+1, zone->half_y);
            armx = ImVec2(vertex.x,vertex.y+zone->half_width+1);
            army = ImVec2(vertex.x-(zone->half_width+1),vertex.y);
            break;
        case ZONE_SHAPE_TRIANGLE_BOTTOM_LEFT:
            vertex = ImVec2(zone->half_x, zone->half_y+1);
            armx = ImVec2(vertex.x,vertex.y-(zone->half_width+1));
            army = ImVec2(vertex.x+zone->half_width+1,vertex.y);
            break;
    }
    #pragma warning(default : 244)
}

float Area(const ImVec2& A, const ImVec2& B, const ImVec2& C) {
    return std::abs((A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y)) / 2.0f);
}
static bool PointInTriangle(ImVec2 point, ImVec2 vertex, ImVec2 armx, ImVec2 army){
    float triArea = Area(vertex, armx, army);
    
    float area1 = Area(point, vertex, armx);
    float area2 = Area(point, armx, army);
    float area3 = Area(point, army, vertex);
    
    return (0.1f > abs((area1 + area2 + area3)-triArea));
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