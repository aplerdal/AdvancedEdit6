#include "map.hpp"

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <span>
#include "gbalzss.hpp"
#include "imgui.h"
#include "editor.hpp"
#include "tileset.hpp"


std::string Map::get_name(){
    return "Map";
}

void Map::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Map", &open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
    
    // Map Menu Bar
    if (ImGui::BeginMenuBar()){
        if (ImGui::BeginMenu("view")){
            if (ImGui::MenuItem("Reset View")){
                translation = ImVec2();
                scale = 1.0f;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    

    // Draw Track Image
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();
    ImVec2 cursor_pos = ImVec2(win_pos.x + translation.x, win_pos.y + translation.y);
    ImVec2 track_size = ImVec2((as->game_ctx.track_width*TILE_SIZE*scale), (as->game_ctx.track_height*TILE_SIZE*scale));
    if (scale < 1.0f) 
        SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_LINEAR);
    else
        SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_NEAREST);
    ImGui::SetCursorScreenPos(cursor_pos);
    ImGui::Image((ImTextureID)(intptr_t)as->editor_ctx.map_buffer, ImVec2(track_size.x,track_size.y));
    
    // Handle Image Input
    ImVec2 mouse_pos = ImGui::GetMousePos();
    if (ImGui::IsItemHovered()){
        ImVec2 hovered_tile = ImVec2(
            (float)(int)((mouse_pos.x-cursor_pos.x)/(TILE_SIZE*scale)),
            (float)(int)((mouse_pos.y-cursor_pos.y)/(TILE_SIZE*scale))
        );
        ImVec2 abs_hovered_tile = ImVec2(
            cursor_pos.x + hovered_tile.x*(TILE_SIZE*scale),
            cursor_pos.y + hovered_tile.y*(TILE_SIZE*scale)
        );
        if (as->editor_ctx.selected_tile > -1) {
            int tile = as->editor_ctx.selected_tile;
            int tile_x = tile%16;
            int tile_y = tile/16;
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)){
                Map::draw_tile(as, (int)hovered_tile.x, (int)hovered_tile.y, tile);
                as->editor_ctx.layout_buffer[(int)hovered_tile.y*TILEMAP_UNIT*as->game_ctx.track_width + (int)hovered_tile.x] = (uint8_t)tile;
            }
            ImVec2 tile_atlas_pos = ImVec2(tile_x*(1.0f/16), tile_y*(1.0f/16));
            ImGui::SetCursorScreenPos(abs_hovered_tile);
            ImGui::Image((ImTextureID)(intptr_t)as->editor_ctx.tile_buffer,ImVec2((TILE_SIZE*scale),(TILE_SIZE*scale)), 
                tile_atlas_pos,
                ImVec2(tile_atlas_pos.x + (1.0f/16), tile_atlas_pos.y + (1.0f/16))
            );
        }
        
        
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
            dragging = true;
            drag_pos = mouse_pos;
            drag_map_pos = translation;
        }
        if (as->editor_ctx.scroll_wheel != 0.0f){
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 relative_mouse_pos = ImVec2(mouse_pos.x - cursor_pos.x, mouse_pos.y - cursor_pos.y);
            float zoom_factor = 1.25f;
            if (as->editor_ctx.scroll_wheel > 0) {
                scale *= zoom_factor;
            } else {
                scale /= zoom_factor;
            }
            ImVec2 new_track_size = ImVec2(as->game_ctx.track_width * TILE_SIZE * scale, as->game_ctx.track_height * TILE_SIZE * scale);
            translation.x += (relative_mouse_pos.x - (relative_mouse_pos.x * (new_track_size.x / track_size.x)));
            translation.y += (relative_mouse_pos.y - (relative_mouse_pos.y * (new_track_size.y / track_size.y)));
        }
    }
    if (dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)){
        dragging = false;
    }
    if (dragging){
        ImVec2 mouse_pos = ImGui::GetMousePos();
        translation = ImVec2(drag_map_pos.x-(drag_pos.x-mouse_pos.x), drag_map_pos.y-(drag_pos.y-mouse_pos.y));
    }

    ImGui::End();
}

void Map::draw_tile(AppState* as, int x, int y, int tile){
    SDL_SetRenderTarget(as->renderer, as->editor_ctx.map_buffer);
    SDL_FRect src = { (float)(TILE_SIZE*(tile%16)), (float)(TILE_SIZE*(tile/16)), TILE_SIZE, TILE_SIZE };
    SDL_FRect dest = {(float)x*TILE_SIZE,(float)y*TILE_SIZE, TILE_SIZE, TILE_SIZE};
    SDL_RenderTexture(as->renderer, as->editor_ctx.tile_buffer, &src, &dest);

    SDL_SetRenderTarget(as->renderer, NULL);
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
                std::vector v = LZSS::lz10_decode(data,true);
                std::copy(v.begin(), v.end(), &(layout_buffer.data()[i*0x1000]));
            }
        }
    } else {
        std::span<uint8_t> data(layout, (uintptr_t)as->game_ctx.eof - (uintptr_t)layout);
        std::vector v = LZSS::lz10_decode(data,true);
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
    SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_NEAREST);


    SDL_SetRenderTarget(as->renderer, as->editor_ctx.map_buffer);

    SDL_FRect src;
    SDL_FRect dest {0,0,TILE_SIZE,TILE_SIZE};
    SDL_Texture* tiles = as->editor_ctx.tile_buffer;
    for (int y = 0; y < track_height; y++) {
        for (int x = 0; x < track_width; x++) {
            int tile = layout_buffer[y*track_height + x];
            src = { (float)(TILE_SIZE*(tile%16)), (float)(TILE_SIZE*(tile/16)), TILE_SIZE, TILE_SIZE };
            SDL_RenderTexture(as->renderer, tiles, &src, &dest);
            dest.x += TILE_SIZE;
        }
        dest.x = 0;
        dest.y += TILE_SIZE;
    }

    SDL_SetRenderTarget(as->renderer, NULL);
}
