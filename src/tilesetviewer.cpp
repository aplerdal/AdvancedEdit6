#include "tilesetviewer.hpp"
#include "imgui.h"

std::string TilesetViewer::get_name(){
    return "Tileset View";
}

void TilesetViewer::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Track List", &open);
    ImGui::End();
}

static void generate_cache(){
    
}