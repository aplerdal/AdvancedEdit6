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
#include "graphics.hpp"


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
    
    ImGui::SetCursorScreenPos(state.cursor_pos);
    ImGui::Image((ImTextureID)(intptr_t)as->editor_ctx.map_buffer, ImVec2(state.track_size.x,state.track_size.y));
    
    if (ImGui::IsWindowFocused()){
        as->editor_ctx.inspector = this;
        if (was_focused) {
            if (state.scale < 1.0f) 
                SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_LINEAR);
            else
                SDL_SetTextureScaleMode(as->editor_ctx.map_buffer, SDL_SCALEMODE_NEAREST);
    
            // Handle Tool input
            view->update(as, state);
            active_tool->update(as, state);
    
            if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) {
                undo(as);
            }
            if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y)) {
                redo(as);
            }
        } else { was_focused = ImGui::IsWindowFocused(); }
    } else {

    }
    ImGui::End();
}
void Tilemap::inspector(AppState* as) {
    ImGui::Text("Tilemap Inspector");
    ImGui::Separator();
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

    ImGui::Separator();
    ImGui::Text("Import/Export Tileset:");
    if (ImGui::Button("Import")) {
        SDL_ShowOpenFileDialog(OpenTilesetCallback, as, as->window, imageFilter, 2, NULL, false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Export")) {
        SDL_ShowSaveFileDialog(SaveTilesetCallback, as, as->window, imageFilter, 2, NULL);
    }
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
    generate_tile_cache(as, track);
    
    track_header_t* header = as->game_ctx.tracks[track].track_header;
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
void Tilemap::generate_tile_cache(AppState* as, int track){
    track_header_t* header = as->game_ctx.tracks[track].track_header;
    uint8_t* base = (uint8_t*)header;

    std::vector<uint8_t> raw_tiles(4 * 0x1000);
    BGR* tile_pal = (BGR*)(base + header->palette_offset);
    uint8_t* tile_header = (uint8_t*)(base + header->tileset_offset);
    uint32_t split_tileset = header->track_flags & TRACK_FLAGS_SPLIT_TILESET;
    if (header->reused_tileset != 0){
        int reused_track = (int)(uint8_t)((uint8_t)track + (uint8_t)header->reused_tileset);
        track_header_t* reused_header = as->game_ctx.tracks[reused_track].track_header;
        uint8_t* reused_base = (uint8_t*)reused_header;
        tile_header = (uint8_t*)(reused_base + reused_header->tileset_offset);
        split_tileset = reused_header->track_flags & TRACK_FLAGS_SPLIT_TILESET;
    }
    if (split_tileset) {
        for (int i = 0; i < 4; i++){
            if (((uint16_t*)tile_header)[i] != 0){
                uint8_t* addr = (uint8_t*)(tile_header+((uint16_t*)tile_header)[i]);
                std::span<uint8_t> data(addr, (uintptr_t)as->game_ctx.eof - (uintptr_t)addr);
                std::vector v = LZSS::lz10_decode(data,true);
                std::copy(v.begin(), v.end(), &(raw_tiles.data()[i*0x1000]));
            }
        }
    } else {
        std::span<uint8_t> data(tile_header, as->game_ctx.eof - tile_header);
        std::vector v = LZSS::lz10_decode(data,true);
        std::copy(v.begin(), v.end(), raw_tiles.data());
    }
    SDL_Palette* palette = SDL_CreatePalette(64);
    SDL_Color pal_buf[64];
    for (int i = 0; i < 64; i++) {
        pal_buf[i] = SDL_Color{
            (uint8_t)((tile_pal[i]<<3)&0b11111000),
            (uint8_t)((tile_pal[i]>>2)&0b11111000),
            (uint8_t)((tile_pal[i]>>7)&0b11111000),
            0xff
        };
    }
    SDL_SetPaletteColors(palette, pal_buf, 0, 64);
    as->editor_ctx.palette = palette;

    SDL_DestroySurface(as->editor_ctx.tile_surface);
    as->editor_ctx.tile_surface = SDL_CreateSurface(16*TILE_SIZE, 16*TILE_SIZE, SDL_PIXELFORMAT_INDEX8);

    SDL_SetSurfacePalette(as->editor_ctx.tile_surface, palette);
    for (int y = 0; y<16; y++){
        for (int x = 0; x<16; x++){
            SDL_Surface* temp = Graphics::decode_8bpp(&raw_tiles.data()[(y*16+x)*64],palette);
            SDL_Rect dest = { x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE };
            SDL_BlitSurface(temp, NULL, as->editor_ctx.tile_surface, &dest);
            SDL_DestroySurface(temp);
        }
    }
    if (as->editor_ctx.tile_buffer != nullptr) {
        SDL_DestroyTexture(as->editor_ctx.tile_buffer);
    }

    as->editor_ctx.tile_buffer = SDL_CreateTextureFromSurface(as->renderer, as->editor_ctx.tile_surface);
    if (!as->editor_ctx.tile_buffer) {
        ImGui::DebugLog("ERROR: Failed to create texture from surface for tileset: %s\n", SDL_GetError());
    }
    SDL_SetTextureScaleMode(as->editor_ctx.tile_buffer, SDL_SCALEMODE_NEAREST);
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

static void SDLCALL SaveTilesetCallback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*filelist) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
        return;
    }
    AppState* as = (AppState*)userdata;
    SDL_SaveBMP(as->editor_ctx.tile_surface, *filelist);

}
static void SDLCALL OpenTilesetCallback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*filelist) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
        return;
    }

    SDL_Surface* original = SDL_LoadBMP(*filelist);
    if (!original) { SDL_Log("Error loading image"); SDL_DestroySurface(original); return; }
    
    if (original->format != SDL_PIXELFORMAT_INDEX8) {
        SDL_Log("Image must be in 8bpp paletted mode. This is usually done by modifying an exported tileset.");
        SDL_DestroySurface(original);
        return;
    }
    
}