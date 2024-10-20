#include "tracklist.hpp"
#include "imgui.h"
#include "lzss.hpp"
#include "graphics.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <algorithm>

std::string TrackList::get_name(){
    return "Track List";
}
static void load_gfx_buffer(AppState* as, int track);
void TrackList::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Track List", &open);
    if (as->editor_ctx.file.size() == 0){
        ImGui::Text("No file opened");
        ImGui::End();
        return;
    }
    ImGui::Text(as->game_ctx.track_table->date);
    int i = 0;
    for (int page = 0; page < pagesCount; page++){
        ImGui::PushID(page);
        if (ImGui::TreeNode(pagesList[page])){
            for (int cup = 0; cup < cupsCount/pagesCount; cup++){
                ImGui::PushID(cup);
                if (ImGui::TreeNode(cupsList[cup+page*(cupsCount/pagesCount)])){
                    for (int track = 0; track < tracksCount/cupsCount; track++){
                        ImGui::PushID(track);
                        if (ImGui::Button(tracksList[track+(cup+page*(cupsCount/pagesCount))*tracksCount/cupsCount])) {
                            load_gfx_buffer(as, track+(cup+page*(cupsCount/pagesCount))*tracksCount/cupsCount);
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::End();
}

static void load_gfx_buffer(AppState* as, int track){
    printf("ldgfx\n");
    as->editor_ctx.selected_track = track;
    TrackHeader* header = as->game_ctx.track_headers[track];
    uint8_t* base = (uint8_t*)header;
    std::vector<uint8_t> raw_tiles(4 * 0x1000);
    BGR* pal = (BGR*)(base + header->palette_offset);
    uint8_t* tile_header = (uint8_t*)(base + header->tileset_offset);
    if (header->track_flags & TRACK_FLAGS_SPLIT_TILESET) {
        for (int i = 0; i < 4; i++){
            if (((uint16_t*)tile_header)[i] != 0){
                uint8_t* addr = (uint8_t*)(tile_header+((uint16_t*)tile_header)[i]);
                std::span<uint8_t> data(addr, (uintptr_t)as->game_ctx.eof - (uintptr_t)addr);
                std::vector v = LZSS::Decode(data);
                std::copy(v.begin(), v.end(), &(raw_tiles.data()[i*0x1000]));
            }
        }
    } else {
        std::span<uint8_t> data(tile_header, as->game_ctx.eof - tile_header);
        std::vector v = LZSS::Decode(data);
        std::copy(v.begin(), v.end(), raw_tiles.data());
    }
    for (int i = 0; i<0x4000/64; i++){
        SDL_Surface* surface = Graphics::decode_8bpp(&raw_tiles.data()[i*64],pal);
        if (as->editor_ctx.tile_buffer[i] != nullptr) {
            SDL_DestroyTexture(as->editor_ctx.tile_buffer[i]);
        }
        as->editor_ctx.tile_buffer[i] = SDL_CreateTextureFromSurface(as->renderer, surface);
        SDL_SetTextureScaleMode(as->editor_ctx.tile_buffer[i], SDL_SCALEMODE_NEAREST);
        if (!as->editor_ctx.tile_buffer[i]) {
            printf("Failed to create texture from surface for tile %d: %s\n", i, SDL_GetError());
        }
        SDL_DestroySurface(surface);
    }
}