#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <list>
#include <utility>
#include "editor.hpp"
#include "imgui.h"

#include "tools.hpp"

#define UNDO_HISTORY_SIZE 16

#define TILEMAP_UNIT 128

static const SDL_DialogFileFilter imageFilter[] = {
    { "All images",  "bmp" },
    { "All files",   "*" }
};

class Tilemap : public Scene {
public:
    Tilemap();
    void update(AppState* as) override;
    void inspector(AppState* as) override;
    void undo(AppState* as);
    void redo(AppState* as);
    static void generate_cache(AppState* as, int track);
    static void generate_tile_cache(AppState* as, int track);
    static void draw_tile(AppState* as, int x, int y, int tile);
    static void regen_map_texture(AppState* as);
    std::string get_name() override;
private:
    ViewTool* view;
    std::vector<Tool*> tools;
    MapState state;
    Tool* active_tool;
    bool was_focused;
};

static void SDLCALL OpenTilesetCallback(void* userdata, const char* const* filelist, int filter);