#pragma once
#include "editor.hpp"

#define TILESET_SIZE 16
#define TILE_DISP_SIZE 16
#define TILE_SIZE 8

class Tileset : public Scene {
public:
    void update(AppState* as) override;
    static void generate_cache(AppState* as, int track);
    std::string get_name() override;
};