#include "tileset.hpp"

#include "imgui.h"
#include "graphics.hpp"
#include <vector>
#include <span>
#include "lzss.hpp"


std::string Tileset::get_name(){
    return "Tileset";
}

void Tileset::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Tileset", &open);
    if (as->editor_ctx.selected_track < 0 || !as->editor_ctx.file_open) {
        ImGui::Text("No track loaded");
        ImGui::End();
        return;
    }
    auto tex = as->editor_ctx.tile_buffer;
    if (tex == nullptr)  {
        ImGui::End();
        return;
    }
    ImVec2 pos = ImGui::GetCursorPos();
    float tile_offset = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
    for (int y = 0; y < TILESET_SIZE; y++){
        ImGui::PushID(y);
        for (int x = 0; x < TILESET_SIZE; x++){
            ImGui::PushID(x);
            ImGui::SetCursorPos(ImVec2(pos.x+TILE_DISP_SIZE*x, pos.y+TILE_DISP_SIZE*y));
            ImVec2 hvr_pos = ImGui::GetCursorScreenPos();
            if (ImGui::ImageButton("tileset", (ImTextureID)(intptr_t)tex, ImVec2(TILE_DISP_SIZE,TILE_DISP_SIZE), ImVec2(tile_offset,0), ImVec2(tile_offset+((float)TILE_SIZE/(256*8)),1))){
                as->editor_ctx.selected_tile = x + y*16;
            }
            tile_offset += ((float)TILE_SIZE/(256*8));
            if (ImGui::IsItemHovered()){
                ImGui::GetForegroundDrawList()->AddRect(
                    ImVec2(hvr_pos.x-2,hvr_pos.y-2),
                    ImVec2(hvr_pos.x+TILE_DISP_SIZE+2,hvr_pos.y+TILE_DISP_SIZE+2),
                    ImGui::GetColorU32(ImGuiCol_ButtonHovered),
                    0.0f,
                    0,
                    2.0f
                );
            }
            if (x + y*16 == as->editor_ctx.selected_tile) {
                ImGui::GetForegroundDrawList()->AddRect(
                    ImVec2(hvr_pos.x-2,hvr_pos.y-2),
                    ImVec2(hvr_pos.x+TILE_DISP_SIZE+2,hvr_pos.y+TILE_DISP_SIZE+2),
                    ImGui::GetColorU32(ImGuiCol_ButtonActive),
                    0.0f,
                    0,
                    2.0f
                );
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

    SDL_Surface* buf_surface = SDL_CreateSurface(256*TILE_SIZE, TILE_SIZE, SDL_PIXELFORMAT_XBGR1555);
    for (int i = 0; i<256; i++){
        SDL_Surface* temp = Graphics::decode_8bpp(&raw_tiles.data()[i*64],pal);
        SDL_Rect dest = { i*TILE_SIZE, 0, TILE_SIZE, TILE_SIZE };
        SDL_BlitSurface(temp, NULL, buf_surface, &dest);
        SDL_DestroySurface(temp);
    }
    if (as->editor_ctx.tile_buffer != nullptr) {
        SDL_DestroyTexture(as->editor_ctx.tile_buffer);
    }
    as->editor_ctx.tile_buffer = SDL_CreateTextureFromSurface(as->renderer, buf_surface);
    if (!as->editor_ctx.tile_buffer) {
        printf("Failed to create texture from surface for tileset: %s\n", SDL_GetError());
    }
    SDL_SetTextureScaleMode(as->editor_ctx.tile_buffer, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(buf_surface);
}