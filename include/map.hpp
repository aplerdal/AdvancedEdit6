#pragma once

#include <SDL3/SDL.h>
#include "editor.hpp"

class Map : public Scene {
public:
    void update(AppState* as) override;
    static void generate_cache(AppState* as, int track);
    std::string get_name() override;
};