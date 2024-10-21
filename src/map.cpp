#include "map.hpp"

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <span>
#include "lzss.hpp"
#include "imgui.h"
#include "editor.hpp"

std::string Map::get_name(){
    return "Map";
}

void Map::update(AppState* as){
    if (!open || !as->editor_ctx.file_open || as->editor_ctx.selected_track < 0) return;
    ImGui::Begin("Map", &open);
    ImGui::Text("Map goes here");
    ImGui::End();
}
void Map::generate_cache(AppState* as, int track) {
    TrackHeader* header = as->game_ctx.track_headers[track];
    uint8_t* base = (uint8_t*)header;
    uint8_t* layout = (uint8_t*)(base + header->layout_offset);
    int track_width = header->width;
    int track_height = header->height;
    if (as->editor_ctx.map_buffer != nullptr){
        float w, h;
        SDL_GetTextureSize(as->editor_ctx.map_buffer, &w, &h);
        if (w != 8*128*track_width || h != 8*128*track_height) {
            SDL_DestroyTexture(as->editor_ctx.map_buffer);
            as->editor_ctx.map_buffer = SDL_CreateTexture(
                as->renderer,
                SDL_PIXELFORMAT_XBGR1555, 
                SDL_TEXTUREACCESS_TARGET, 
                8*128*track_width, 
                8*128*track_height
            );
            printf("Resized map texture");
        }
    } else {
        as->editor_ctx.map_buffer = SDL_CreateTexture(
            as->renderer,
            SDL_PIXELFORMAT_XBGR1555, 
            SDL_TEXTUREACCESS_TARGET, 
            8*128*track_width, 
            8*128*track_height
        );
    }

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

    SDL_SetRenderTarget(as->renderer, as->editor_ctx.map_buffer);

    // Do rendering.

    SDL_SetRenderTarget(as->renderer, NULL);
}
