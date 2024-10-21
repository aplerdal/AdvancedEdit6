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
};