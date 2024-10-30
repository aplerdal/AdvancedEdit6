#include "tilemap.hpp"

#include <SDL3/SDL.h>
#include "imgui.h"
#include "gbalzss.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <span>

#include "editor.hpp"

#include "tileset.hpp"
#include "tools.hpp"


std::string Tilemap::get_name(){
    return "Tilemap";
}
Tilemap::Tilemap(){
    view = new ViewTool();
    tools = {
        new DrawTool,
    };
    active_tool = tools[0];
}
void Tilemap::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Tilemap", &open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
    
    // Tilemap Menu Bar
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
        if (ImGui::BeginMenu("Debug")){
            if (ImGui::MenuItem("Regen Buffer")){
                Tilemap::regen_map_texture(as);
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
    if (state.scale < 1.0f) 
        SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_LINEAR);
    else
        SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_NEAREST);
    ImGui::SetCursorScreenPos(state.cursor_pos);
    ImGui::Image((ImTextureID)(intptr_t)as->editor_ctx.map_buffer, ImVec2(state.track_size.x,state.track_size.y));
    
    // Handle Tools
    view->update(as, state);
    active_tool->update(as, state);

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) {
        undo(as);
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y)) {
        redo(as);
    }
    
    ImGui::End();
}
void Tilemap::undo(AppState *as)
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
void Tilemap::redo(AppState *as)
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
void Tilemap::draw_tile(AppState *as, int x, int y, int tile)
{
    SDL_SetRenderTarget(as->renderer, as->editor_ctx.map_buffer);
    SDL_FRect src = { (float)(TILE_SIZE*(tile%16)), (float)(TILE_SIZE*(tile/16)), TILE_SIZE, TILE_SIZE };
    SDL_FRect dest = {(float)x*TILE_SIZE,(float)y*TILE_SIZE, TILE_SIZE, TILE_SIZE};
    SDL_RenderTexture(as->renderer, as->editor_ctx.tile_buffer, &src, &dest);

    SDL_SetRenderTarget(as->renderer, NULL);
}
void Tilemap::generate_cache(AppState* as, int track) {
    TrackHeader* header = as->game_ctx.track_headers[track];
    uint8_t* base = (uint8_t*)header;
    uint8_t* layout = (uint8_t*)(base + header->layout_offset);
    int track_width = header->width * TILEMAP_UNIT;
    int track_height = header->height * TILEMAP_UNIT;

    as->game_ctx.track_width = track_width;
    as->game_ctx.track_height = track_height;

    std::vector<uint8_t> layout_buffer(track_width*track_height);
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
    regen_map_texture(as);
}
void Tilemap::regen_map_texture(AppState* as){
    int track_width = as->game_ctx.track_width;
    int track_height = as->game_ctx.track_height;
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
            int tile = as->editor_ctx.layout_buffer[y*track_height + x];
            src = { (float)(TILE_SIZE*(tile%16)), (float)(TILE_SIZE*(tile/16)), TILE_SIZE, TILE_SIZE };
            SDL_RenderTexture(as->renderer, tiles, &src, &dest);
            dest.x += TILE_SIZE;
        }
        dest.x = 0;
        dest.y += TILE_SIZE;
    }

    SDL_SetRenderTarget(as->renderer, NULL);
}

void ViewTool::update(AppState *as, MapState& ms)
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    if (ImGui::IsItemHovered()){
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
            dragging = true;
            drag_pos = mouse_pos;
            drag_map_pos = ms.translation;
        }
        if (as->editor_ctx.scroll_wheel != 0.0f){
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 relative_mouse_pos = ImVec2(mouse_pos.x - ms.cursor_pos.x, mouse_pos.y - ms.cursor_pos.y);
            float zoom_factor = 1.25f;
            if (as->editor_ctx.scroll_wheel > 0) {
                ms.scale *= zoom_factor;
            } else {
                ms.scale /= zoom_factor;
            }
            ImVec2 new_track_size = ImVec2(as->game_ctx.track_width * TILE_SIZE * ms.scale, as->game_ctx.track_height * TILE_SIZE * ms.scale);
            ms.translation.x += (relative_mouse_pos.x - (relative_mouse_pos.x * (new_track_size.x / ms.track_size.x)));
            ms.translation.y += (relative_mouse_pos.y - (relative_mouse_pos.y * (new_track_size.y / ms.track_size.y)));
        }
    }
    if (dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)){
        dragging = false;
    }
    if (dragging){
        ms.translation = ImVec2(drag_map_pos.x-(drag_pos.x-mouse_pos.x), drag_map_pos.y-(drag_pos.y-mouse_pos.y));
    }
}

DrawCmd::DrawCmd(AppState* as, TileBuffer tile_buf){
    new_tiles = tile_buf;
    old_tiles = TileBuffer();
    for (auto const& [pos,tile] : new_tiles) {
        old_tiles[pos] = as->editor_ctx.layout_buffer[pos.y*as->game_ctx.track_width + pos.x];
    }
}
void DrawCmd::execute(AppState* as) {
    for (auto const& [pos,tile] : new_tiles) {
        as->editor_ctx.layout_buffer[pos.y*as->game_ctx.track_width + pos.x] = tile;
    }
}
void DrawCmd::redo(AppState* as) {
    for (auto const& [pos,tile] : new_tiles) {
        Tilemap::draw_tile(as, pos.x, pos.y, tile);
        as->editor_ctx.layout_buffer[pos.y*as->game_ctx.track_width + pos.x] = tile;
    }
}
void DrawCmd::undo(AppState* as) {
    SDL_SetRenderTarget(as->renderer, as->editor_ctx.map_buffer);

    for (auto const& [pos,tile] : old_tiles) {
        SDL_FRect src = { (float)(TILE_SIZE*(tile%16)), (float)(TILE_SIZE*(tile/16)), TILE_SIZE, TILE_SIZE };
        SDL_FRect dest = { (float)pos.x*TILE_SIZE,(float)pos.y*TILE_SIZE, TILE_SIZE, TILE_SIZE };
        SDL_RenderTexture(as->renderer, as->editor_ctx.tile_buffer, &src, &dest);
        as->editor_ctx.layout_buffer[pos.y*as->game_ctx.track_width + pos.x] = tile;
    }
    
    SDL_SetRenderTarget(as->renderer, NULL);
    //Tilemap::generate_cache(as, as->editor_ctx.selected_track);
}

void DrawTool::update(AppState *as, MapState& ms)
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    if (ImGui::IsItemHovered()){
        vec2i hovered_tile = {
            (int)((mouse_pos.x-ms.cursor_pos.x)/(TILE_SIZE*ms.scale)),
            (int)((mouse_pos.y-ms.cursor_pos.y)/(TILE_SIZE*ms.scale))
        };
        ImVec2 abs_hovered_tile = ImVec2(
            ms.cursor_pos.x + hovered_tile.x*(TILE_SIZE*ms.scale),
            ms.cursor_pos.y + hovered_tile.y*(TILE_SIZE*ms.scale)
        );
        if (as->editor_ctx.selected_tile > -1) {
            int tile = as->editor_ctx.selected_tile;
            vec2i tile_pos = { tile%16, tile/16 };
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)){
                if (!draw_buf.contains({hovered_tile.x, hovered_tile.y})) {
                    Tilemap::draw_tile(as, hovered_tile.x, hovered_tile.y, tile);
                    draw_buf.insert(std::pair<vec2i, uint8_t>({hovered_tile.x, hovered_tile.y}, tile));
                }
                held = true;
            } else if (held)
            {
                held = false;
                auto cmd = new DrawCmd(as, draw_buf);

                PUSH_STACK(as->editor_ctx.undo_stack, cmd);
                as->editor_ctx.redo_stack = std::deque<Command*>();
                
                draw_buf = TileBuffer();
                
                cmd->execute(as);
            }
            
            ImVec2 tile_atlas_pos = ImVec2(tile_pos.x*(1.0f/16), tile_pos.y*(1.0f/16));
            ImGui::SetCursorScreenPos(abs_hovered_tile);
            ImGui::Image((ImTextureID)(intptr_t)as->editor_ctx.tile_buffer,ImVec2((TILE_SIZE*ms.scale),(TILE_SIZE*ms.scale)), 
                tile_atlas_pos,
                ImVec2(tile_atlas_pos.x + (1.0f/16), tile_atlas_pos.y + (1.0f/16))
            );
        }
    }
}