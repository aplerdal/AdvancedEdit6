#include "tilesetviewer.hpp"
#include "imgui.h"
#include "graphics.hpp"

std::string TilesetViewer::get_name(){
    return "Tileset View";
}

#define TILESET_SIZE 16
#define TILE_DISP_SIZE 16
#define TILE_SIZE 16

void TilesetViewer::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Tileset View", &open);
    if (as->editor_ctx.selected_track < 0 || as->editor_ctx.file.size() == 0) {
        ImGui::Text("No track loaded");
        ImGui::End();
        return;
    }
    ImVec2 pos = ImGui::GetCursorPos();
    for (int y = 0; y < TILESET_SIZE; y++){
        for (int x = 0; x < TILESET_SIZE; x++){
            auto tex = as->editor_ctx.tile_buffer[y*TILESET_SIZE+x];
            if (tex != nullptr) {
                ImGui::SetCursorPos(ImVec2(pos.x+TILE_SIZE*x, pos.y+TILE_SIZE*y));
                ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(TILE_SIZE,TILE_SIZE));
            } else {

            }
        }    
    }
    ImGui::End();
}