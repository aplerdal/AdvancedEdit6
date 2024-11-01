#include "ai.hpp"

#include <SDL3/SDL.h>

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
    if (ImGui::IsWindowFocused()){
        if (state.scale < 1.0f) 
            SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_LINEAR);
        else
            SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_NEAREST);
    }
    ImGui::SetCursorScreenPos(state.cursor_pos);
    ImGui::Image((ImTextureID)(intptr_t)as->editor_ctx.map_buffer, ImVec2(state.track_size.x,state.track_size.y));
    DrawAILayout(as);
    // Handle Tools
    view->update(as, state);

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) {
        undo(as);
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y)) {
        redo(as);
    }
    
    ImGui::End();
}
void AI::DrawAILayout(AppState *as){
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
    AI::SectorInput(as, t);
    AI::SectorDraw(dl, t);

    dl->PopClipRect();
}
void AI::SectorInput(AppState *as, TrackContext* t) {
    ImVec2 mouse_pos = ImGui::GetMousePos();
    hovered_sector = -1;
    target_hovered = false;
    for (int i = 0; i < t->ai_header->count; i++) {
        auto zone = t->ai_zones[i];
        auto target = t->ai_targets[0][i];

        float sel_circle_rad = CIRCLE_RAD * state.scale;
        if (zone->shape == ZONE_SHAPE_RECTANGLE) {
            auto min = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            auto max = ImVec2(min.x + (state.scale*zone->half_width*TILE_SIZE*2.0f), min.y + (state.scale*zone->half_height*TILE_SIZE*2.0f));
        
            if (PointInRect(mouse_pos, min, max) && !target_hovered){
                hovered_sector = i;
            }
        } else {
            ImVec2 vertex = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            float tri_size = state.scale*zone->half_width*TILE_SIZE*2.0f;
            
            if (PointInTriangle(mouse_pos, vertex, zone->shape, tri_size) && !target_hovered) {
                hovered_sector = i;
            }
        }

        ImVec2 target_pos = ImVec2(state.cursor_pos.x+(state.scale*target->x*TILE_SIZE), state.cursor_pos.y+(state.scale*target->y*TILE_SIZE));

        
        if (std::hypot(target_pos.x-mouse_pos.x, target_pos.y - mouse_pos.y) < sel_circle_rad) {
            hovered_sector = i;
            target_hovered = true;
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (!dragging) {
                    dragging = true;
                    drag_sector = i;
                    is_dragging_target = true;
                    drag_start = ImVec2(
                        (uint16_t)((mouse_pos.x - state.cursor_pos.x)/(state.scale * TILE_SIZE)),
                        (uint16_t)((mouse_pos.y - state.cursor_pos.y)/(state.scale * TILE_SIZE))
                    );
                }
            }
        }
        if (dragging && drag_sector == i && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            target->x = (uint16_t)((mouse_pos.x - state.cursor_pos.x)/(state.scale * TILE_SIZE));
            target->y = (uint16_t)((mouse_pos.y - state.cursor_pos.y)/(state.scale * TILE_SIZE));
        }
        if (dragging && drag_sector == i && ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
            PUSH_STACK(
                as->editor_ctx.undo_stack,
                new TranslateCmd(
                    as, drag_sector, is_dragging_target,drag_start,
                    ImVec2((float)target->x,(float)target->y)
                )
            );
            dragging = false;
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

        bool hovered = (i==hovered_sector && !target_hovered );
        float border_scale = hovered? HOVER_BORDER_SIZE:1.0f;
        float sel_circle_rad = CIRCLE_RAD * state.scale;

        if (zone->shape == ZONE_SHAPE_RECTANGLE) {
            auto min = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            auto max = ImVec2(min.x + (state.scale*zone->half_width*TILE_SIZE*2.0f), min.y + (state.scale*zone->half_height*TILE_SIZE*2.0f));

            dl->AddRectFilled(min, max, hovered?hover_color:fill_color);
            dl->AddRect(min,max,border_color, 0.0f, 0, border_scale);
        
            ImVec2 rmin = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            ImVec2 rmax = ImVec2(rmin.x + (state.scale*zone->half_width*TILE_SIZE*2.0f), rmin.y + (state.scale*zone->half_height*TILE_SIZE*2.0f));
            ImVec2 target_pos = ImVec2(state.cursor_pos.x+(state.scale*target->x*TILE_SIZE), state.cursor_pos.y+(state.scale*target->y*TILE_SIZE));
            
            hovered = (i==hovered_sector && target_hovered);
            dl->AddCircleFilled(target_pos, sel_circle_rad, hovered?hover_color:fill_color);
            dl->AddCircle(target_pos, sel_circle_rad, border_color, 0, hovered? HOVER_BORDER_SIZE:1.0f);

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
            ImVec2 vertex = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            float tri_size = state.scale*zone->half_width*TILE_SIZE*2.0f;
            ImVec2 armx, army;
            ZoneArms(zone->shape, vertex, tri_size, armx, army);

            dl->AddTriangleFilled(vertex, armx, army, hovered?hover_color : fill_color);
            dl->AddTriangle(vertex, armx, army, border_color, border_scale);

            // Target
            ImVec2 target_pos = ImVec2(state.cursor_pos.x+(state.scale*target->x*TILE_SIZE), state.cursor_pos.y+(state.scale*target->y*TILE_SIZE));

            float armx_dist = std::hypot(armx.x - target_pos.x, armx.y - target_pos.y);
            float army_dist = std::hypot(army.x - target_pos.x, army.y - target_pos.y);
            float vert_dist = std::hypot(vertex.x - target_pos.x, vertex.y - target_pos.y);

            hovered = (i==hovered_sector && target_hovered);
            dl->AddCircleFilled(target_pos, sel_circle_rad, hovered?hover_color:fill_color);
            dl->AddCircle(target_pos, sel_circle_rad, border_color, 0, hovered? HOVER_BORDER_SIZE:1.0f);

            if (armx_dist > army_dist && armx_dist > vert_dist)
                dl->AddTriangle(vertex, army, target_pos, border_color, border_scale);
            else if (army_dist > armx_dist && army_dist > vert_dist)
                dl->AddTriangle(vertex, armx, target_pos, border_color, border_scale);
            else
                dl->AddTriangle(armx, army, target_pos, border_color, border_scale);
        }
    }
}

static void ZoneArms(uint8_t shape, ImVec2 vertex, float tri_size, ImVec2& armx, ImVec2& army) {
    switch (shape)
    {
        case ZONE_SHAPE_TRIANGLE_BOTTOM_RIGHT:
            armx = ImVec2(vertex.x,vertex.y-tri_size);
            army = ImVec2(vertex.x-tri_size,vertex.y);
            break;
        case ZONE_SHAPE_TRIANGLE_TOP_LEFT:
            armx = ImVec2(vertex.x,vertex.y+tri_size);
            army = ImVec2(vertex.x+tri_size,vertex.y);
            break;
        case ZONE_SHAPE_TRIANGLE_TOP_RIGHT:
            armx = ImVec2(vertex.x,vertex.y+tri_size);
            army = ImVec2(vertex.x-tri_size,vertex.y);
            break;
        case ZONE_SHAPE_TRIANGLE_BOTTOM_LEFT:
            armx = ImVec2(vertex.x,vertex.y-tri_size);
            army = ImVec2(vertex.x+tri_size,vertex.y);
            break;
    }
}

static bool PointInTriangle(ImVec2 point, ImVec2 vertex, uint8_t shape, float size){
    float x = point.x - vertex.x;
    float y = point.y - vertex.y;
    switch (shape)
    {
        case ZONE_SHAPE_TRIANGLE_BOTTOM_RIGHT:
            return (vertex.x - size <= point.x && point.x <= vertex.x) && 
                   (vertex.y - size <= point.y && point.y <= vertex.y) && 
                   (point.y >= -point.x + vertex.x + vertex.y - size);
        case ZONE_SHAPE_TRIANGLE_TOP_LEFT:
            return (vertex.x <= point.x && point.x <= vertex.x + size) && 
                   (vertex.y <= point.y && point.y <= vertex.y + size) && 
                   (point.y <= -point.x + vertex.y + size + vertex.x);
        case ZONE_SHAPE_TRIANGLE_TOP_RIGHT:
            return (vertex.x - size <= point.x && point.x <= vertex.x) && 
                   (vertex.y <= point.y && point.y <= vertex.y + size) && 
                   (point.y <= point.x - vertex.x + vertex.y + size);
        case ZONE_SHAPE_TRIANGLE_BOTTOM_LEFT:
            return (vertex.x <= point.x && point.x <= vertex.x + size) && 
                   (vertex.y - size <= point.y && point.y <= vertex.y) && 
                   (point.y >= point.x - vertex.x + vertex.y - size);
        default: return false;
    }
}
static bool PointInCircle(ImVec2 point, ImVec2 position, float radius) {
    return std::hypot(point.x-position.x, point.y-position.y) <= radius;
}
static bool PointInRect(ImVec2 point, ImVec2 min, ImVec2 max){
    return (point.x >= min.x && point.y >= min.y) && (point.x <= max.x && point.y <= max.y);
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

TranslateCmd::TranslateCmd(AppState* as, int drag_sector, bool dragging_target, ImVec2 old_pos, ImVec2 new_pos)
{
    this->old_pos = old_pos;
    this->new_pos = new_pos;
    this->drag_sector = drag_sector;
    this->dragging_target = dragging_target;
}

void TranslateCmd::execute(AppState *as)
{
    return;
}

void TranslateCmd::redo(AppState *as)
{
    printf("drag sector = %d", drag_sector);
    if (dragging_target) {
        auto target = as->game_ctx.tracks[as->editor_ctx.selected_track].ai_targets[0][drag_sector];
        target->x = (uint16_t)(new_pos.x);
        target->y = (uint16_t)(new_pos.y);
    } else {
        auto zone = as->game_ctx.tracks[as->editor_ctx.selected_track].ai_zones[drag_sector];
        zone->half_x = (uint16_t)(new_pos.x);
        zone->half_y = (uint16_t)(new_pos.y);
    }
}
void TranslateCmd::undo(AppState *as)
{
    if (dragging_target) {
        auto target = as->game_ctx.tracks[as->editor_ctx.selected_track].ai_targets[0][drag_sector];
        target->x = (uint16_t)(old_pos.x);
        target->y = (uint16_t)(old_pos.y);
    } else {
        auto zone = as->game_ctx.tracks[as->editor_ctx.selected_track].ai_zones[drag_sector];
        zone->half_x = (uint16_t)(old_pos.x);
        zone->half_y = (uint16_t)(old_pos.y);
    }
}
