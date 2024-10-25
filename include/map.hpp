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

class Map : public Scene {
public:
    Map();
    void update(AppState* as) override;
    static void generate_cache(AppState* as, int track);
    static void draw_tile(AppState* as, int x, int y, int tile);
    std::string get_name() override;
private:
    ViewTool* view;
    std::vector<Tool*> tools;
    MapState state;
    Tool* active_tool;

    ImVec2 drag_map_pos;
    ImVec2 drag_pos;
};