#include "map.hpp"

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <span>
#include "lzss.hpp"
#include "imgui.h"
#include "editor.hpp"
#include "tileset.hpp"


std::string Map::get_name(){
    return "Map";
}

void Map::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Map", &open, ImGuiWindowFlags_NoScrollbar);
    if (!as->editor_ctx.file_open || as->editor_ctx.selected_track < 0) {
        ImGui::Text("No Track Loaded");
        ImGui::End();
        return;
    }
    if (as->editor_ctx.map_buffer != nullptr) {
        ImGui::SetCursorScreenPos(ImGui::GetWindowPos());
        ImVec2 win_size = ImGui::GetWindowSize();
        ImVec2 track_size = ImVec2((as->game_ctx.track_width*TILE_SIZE*zoom), (as->game_ctx.track_height*TILE_SIZE*zoom));
        ImVec2 img_translation_uvs = ImVec2(translation.x/track_size.x, translation.y/track_size.y);
        ImVec2 img_uvs = ImVec2(
            win_size.x/track_size.x + img_translation_uvs.x,
            win_size.y/track_size.y + img_translation_uvs.y
        );
        ImGui::Image((ImTextureID)(intptr_t)as->editor_ctx.map_buffer, ImVec2(win_size.x,win_size.y),img_translation_uvs,img_uvs);
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
        
        // Handle Image input
        if (ImGui::IsItemHovered()){
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
                dragging = true;
                drag_pos = ImGui::GetMousePos();
                drag_map_pos = translation;
            }
            // WHY DOES THIS NOT READ CORRECTLY AHHHHHHHHHHHHH
            float scroll = ImGui::GetIO().MouseWheel;
            if (fabs(scroll) > 0.01f){
                printf("%f\n",scroll);
                ImVec2 mouse_pos = ImGui::GetMousePos();

                drag_pos = mouse_pos;
                drag_map_pos = translation;
                ImVec2 mouse_in_image = ImVec2(
                    (mouse_pos.x - ImGui::GetWindowPos().x) / (track_size.x * zoom),
                    (mouse_pos.y - ImGui::GetWindowPos().y) / (track_size.y * zoom)
                );
                float zoom_change = 1.1f;  // Zoom factor (adjust for smoother/faster zoom)
                if (scroll > 0.0f) {
                    zoom *= scroll*zoom_change;
                } else if (scroll < 0.0f) {
                    zoom /= scroll*zoom_change;
                }
                translation.x = mouse_in_image.x * track_size.x * zoom - (mouse_pos.x - ImGui::GetWindowPos().x);
                translation.y = mouse_in_image.y * track_size.y * zoom - (mouse_pos.y - ImGui::GetWindowPos().y);
            }
        }
        if (dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)){
            dragging = false;
        }
        if (dragging){
            ImVec2 mouse_pos = ImGui::GetMousePos();
            translation = ImVec2(drag_map_pos.x+(drag_pos.x-mouse_pos.x), drag_map_pos.y+(drag_pos.y-mouse_pos.y));
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f,0.25f,0.25f,1.0f), "Error Reading Map Image!");
    }
    ImGui::End();
}
void Map::generate_cache(AppState* as, int track) {
    TrackHeader* header = as->game_ctx.track_headers[track];
    uint8_t* base = (uint8_t*)header;
    uint8_t* layout = (uint8_t*)(base + header->layout_offset);
    int track_width = header->width * TILEMAP_UNIT;
    int track_height = header->height * TILEMAP_UNIT;

    as->game_ctx.track_width = track_width;
    as->game_ctx.track_height = track_height;

    std::vector<uint8_t> layout_buffer(16 * 0x1000);
    if (header->track_flags & TRACK_FLAGS_SPLIT_LAYOUT) {
        for (int i = 0; i < 16; i++) {
            if (((uint16_t*)layout)[i] != 0){
                uint8_t* addr = (uint8_t*)(layout+((uint16_t*)layout)[i]);
                std::span<uint8_t> data(addr, (uintptr_t)as->game_ctx.eof - (uintptr_t)addr);
                std::vector v = LZSS::Decode(data);
                std::copy(v.begin(), v.end(), &(layout_buffer.data()[i*0x1000]));
            }
        }
    } else {
        std::span<uint8_t> data(layout, (uintptr_t)as->game_ctx.eof - (uintptr_t)layout);
        std::vector v = LZSS::Decode(data);
        std::copy(v.begin(), v.end(), layout_buffer.data());
    }
    as->editor_ctx.layout_buffer = layout_buffer;

    if (as->editor_ctx.map_buffer != nullptr){
        float w, h;
        SDL_GetTextureSize(as->editor_ctx.map_buffer, &w, &h);
        if (w != TILE_SIZE*track_width || h != TILE_SIZE*track_height) {
            SDL_DestroyTexture(as->editor_ctx.map_buffer);
            as->editor_ctx.map_buffer = SDL_CreateTexture(
                as->renderer,
                SDL_PIXELFORMAT_XBGR1555, 
                SDL_TEXTUREACCESS_TARGET, 
                TILE_SIZE*track_width, 
                TILE_SIZE*track_height
            );
            printf("Resized map texture");
        }
    } else {
        as->editor_ctx.map_buffer = SDL_CreateTexture(
            as->renderer,
            SDL_PIXELFORMAT_XBGR1555, 
            SDL_TEXTUREACCESS_TARGET, 
            TILE_SIZE*track_width, 
            TILE_SIZE*track_height
        );
    }


    SDL_SetRenderTarget(as->renderer, as->editor_ctx.map_buffer);

    SDL_FRect src;
    SDL_FRect dest {0,0,TILE_SIZE,TILE_SIZE};
    SDL_Texture* tiles = as->editor_ctx.tile_buffer;
    for (int y = 0; y < track_height; y++) {
        for (int x = 0; x < track_width; x++) {
            src = { (float)(TILE_SIZE*layout_buffer[y*track_height + x]), 0, TILE_SIZE, TILE_SIZE };
            SDL_RenderTexture(as->renderer, tiles, &src, &dest);
            dest.x += TILE_SIZE;
        }
        dest.x = 0;
        dest.y += TILE_SIZE;
    }

    SDL_SetRenderTarget(as->renderer, NULL);
}
