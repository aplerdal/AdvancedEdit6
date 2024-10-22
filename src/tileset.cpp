#include "tileset.hpp"

#include "imgui.h"
#include "graphics.hpp"
#include <vector>
#include <span>
#include "gbalzss.hpp"


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
    SDL_Texture* tex = as->editor_ctx.tile_buffer;
    if (tex == nullptr)  {
        ImGui::Text("Error loading tiles.");
        ImGui::End();
        return;
    }
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    const ImVec2 img_size = ImVec2(TILE_DISP_SIZE*16,TILE_DISP_SIZE*16);
    ImGui::Image((ImTextureID)(intptr_t)tex, img_size);
    if (ImGui::IsItemHovered()){
        ImVec2 hvr_tile = ImVec2 (
            (float)(((int)mouse_pos.x-(int)cursor_pos.x)/TILE_DISP_SIZE),
            (float)(((int)mouse_pos.y-(int)cursor_pos.y)/TILE_DISP_SIZE)
        );
        ImVec2 abs_hvr_tile = ImVec2 (
            hvr_tile.x*TILE_DISP_SIZE + cursor_pos.x,
            hvr_tile.y*TILE_DISP_SIZE + cursor_pos.y
        );
        if (ImGui::IsItemClicked()){
            as->editor_ctx.selected_tile = (int)hvr_tile.x + (int)hvr_tile.y*16;
            printf("%d\n", as->editor_ctx.selected_tile);
        }
        ImGui::GetForegroundDrawList()->AddRect(
            ImVec2(abs_hvr_tile.x-2,abs_hvr_tile.y-2),
            ImVec2(abs_hvr_tile.x+TILE_DISP_SIZE+2,abs_hvr_tile.y+TILE_DISP_SIZE+2),
            ImGui::GetColorU32(ImGuiCol_ButtonHovered),
            0.0f,
            0,
            2.0f
        );
    }
    if (as->editor_ctx.selected_tile > -1) {
        ImGui::GetForegroundDrawList()->AddRect(
            ImVec2(
                cursor_pos.x + (as->editor_ctx.selected_tile%16)*TILE_DISP_SIZE - 2,
                cursor_pos.y + (as->editor_ctx.selected_tile/16)*TILE_DISP_SIZE - 2
            ),
            ImVec2(
                cursor_pos.x + (as->editor_ctx.selected_tile%16)*TILE_DISP_SIZE + TILE_DISP_SIZE + 2,
                cursor_pos.y + (as->editor_ctx.selected_tile/16)*TILE_DISP_SIZE + TILE_DISP_SIZE + 2
            ),
            ImGui::GetColorU32(ImGuiCol_ButtonActive),
            0.0f,
            0,
            2.0f
        );
    }
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
                std::vector v = lzss::lz10_decode(data,true);
                std::copy(v.begin(), v.end(), &(raw_tiles.data()[i*0x1000]));
            }
        }
    } else {
        std::span<uint8_t> data(tile_header, as->game_ctx.eof - tile_header);
        std::vector v = lzss::lz10_decode(data,true);
        std::copy(v.begin(), v.end(), raw_tiles.data());
    }

    SDL_Surface* buf_surface = SDL_CreateSurface(16*TILE_SIZE, 16*TILE_SIZE, SDL_PIXELFORMAT_XBGR1555);
    for (int y = 0; y<16; y++){
        for (int x = 0; x<16; x++){
            SDL_Surface* temp = Graphics::decode_8bpp(&raw_tiles.data()[(y*16+x)*64],pal);
            SDL_Rect dest = { x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE };
            SDL_BlitSurface(temp, NULL, buf_surface, &dest);
            SDL_DestroySurface(temp);
        }
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