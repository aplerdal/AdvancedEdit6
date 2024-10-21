#include "tileset.hpp"

#include "imgui.h"
#include "graphics.hpp"
#include <vector>
#include <span>
#include "lzss.hpp"

std::string Tileset::get_name(){
    return "Tileset";
}

#define TILESET_SIZE 16
#define TILE_DISP_SIZE 16
#define TILE_SIZE 16

void Tileset::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Tileset", &open);
    if (as->editor_ctx.selected_track < 0 || !as->editor_ctx.file_open) {
        ImGui::Text("No track loaded");
        ImGui::End();
        return;
    }
    ImVec2 pos = ImGui::GetCursorPos();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
    for (int y = 0; y < TILESET_SIZE; y++){
        ImGui::PushID(y);
        for (int x = 0; x < TILESET_SIZE; x++){
            ImGui::PushID(x);
            auto tex = as->editor_ctx.tile_buffer[y*TILESET_SIZE+x];
            if (tex != nullptr) {
                ImGui::SetCursorPos(ImVec2(pos.x+TILE_SIZE*x, pos.y+TILE_SIZE*y));
                ImVec2 hvr_pos = ImGui::GetCursorScreenPos();
                if (ImGui::ImageButton("tileset_t",(ImTextureID)(intptr_t)tex, ImVec2(TILE_SIZE,TILE_SIZE))){
                    as->editor_ctx.selected_tile = x + y*16;
                }
                if (ImGui::IsItemHovered()){
                    ImGui::GetForegroundDrawList()->AddRect(
                        ImVec2(hvr_pos.x-2,hvr_pos.y-2),
                        ImVec2(hvr_pos.x+TILE_SIZE+2,hvr_pos.y+TILE_SIZE+2),
                        ImGui::GetColorU32(ImGuiCol_ButtonHovered),
                        0.0f,
                        0,
                        2.0f
                    );

                }
                if (x + y*16 == as->editor_ctx.selected_tile) {
                    ImGui::GetForegroundDrawList()->AddRect(
                        ImVec2(hvr_pos.x-2,hvr_pos.y-2),
                        ImVec2(hvr_pos.x+TILE_SIZE+2,hvr_pos.y+TILE_SIZE+2),
                        ImGui::GetColorU32(ImGuiCol_ButtonActive),
                        0.0f,
                        0,
                        2.0f
                    );
                }
            } else {

            }
            ImGui::PopID(); 
        }
        ImGui::PopID(); 
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

void Tileset::generate_cache(AppState* as, int track){
    TrackHeader* header = as->game_ctx.track_headers[track];
    uint8_t* base = (uint8_t*)header;

    std::vector<uint8_t> raw_tiles(4 * 0x1000);
    BGR* pal = (BGR*)(base + header->palette_offset);
    uint8_t* tile_header = (uint8_t*)(base + header->tileset_offset);
    uint32_t split_tileset = header->track_flags & TRACK_FLAGS_SPLIT_TILESET;
    if (header->reused_tileset != 0){
        int reused_track = (int)(uint8_t)((uint8_t)track + (uint8_t)header->reused_tileset);
        TrackHeader* reused_header = as->game_ctx.track_headers[reused_track];
        uint8_t* reused_base = (uint8_t*)reused_header;
        tile_header = (uint8_t*)(reused_base + reused_header->tileset_offset);
        split_tileset = reused_header->track_flags & TRACK_FLAGS_SPLIT_TILESET;
    }
    if (split_tileset) {
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