#pragma once

#include <SDL3/SDL.h>
#include "editor.hpp"
#include "imgui.h"

#define TILEMAP_UNIT 128

class Map : public Scene {
public:
    void update(AppState* as) override;
    static void generate_cache(AppState* as, int track);
    std::string get_name() override;
private:
    float zoom = 1.0f;
    ImVec2 translation = ImVec2(0.000000001f,0.000000001f);

    bool dragging;
    ImVec2 drag_map_pos;
    ImVec2 drag_pos;
};