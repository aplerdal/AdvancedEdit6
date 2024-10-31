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
    draw_ai_layout(as);
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
void AI::draw_ai_layout(AppState *as){
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
    for (auto zone : t->ai_zones) {
        if (zone->shape == ZONE_SHAPE_RECTANGLE) {
            auto min = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            auto max = ImVec2(min.x + (state.scale*zone->half_width*TILE_SIZE*2.0f), min.y + (state.scale*zone->half_height*TILE_SIZE*2.0f));
            dl->AddRect(min,max, ImColor(0.8f,0.1f,0.7f, 1.0f));
            if (PointInRect(mouse_pos, min, max)){
                dl->AddRectFilled(min, max, ImColor(0.8f,0.1f,0.7f, 0.5f));
            } else {
                dl->AddRectFilled(min, max, ImColor(0.8f,0.1f,0.7f, 0.3f));
            }
        } else {
            ImVec2 vertex = ImVec2(state.cursor_pos.x+(state.scale*zone->half_x*TILE_SIZE*2.0f), state.cursor_pos.y+(state.scale*zone->half_y*TILE_SIZE*2.0f));
            ImVec2 armx;
            ImVec2 army;
            float tri_size = state.scale*zone->half_width*TILE_SIZE*2.0f;
            switch (zone->shape)
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
            dl->AddTriangle(vertex,armx,army,ImColor(0.8f,0.1f,0.7f, 1.0f));
            if (PointInTriangle(mouse_pos, vertex, zone->shape, tri_size)) {
                dl->AddTriangleFilled(vertex,armx,army,ImColor(0.8f,0.1f,0.7f, 0.5f));
            } else {
                dl->AddTriangleFilled(vertex,armx,army,ImColor(0.8f,0.1f,0.7f, 0.3f));
            }
            
        }
    }
    dl->PopClipRect();
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