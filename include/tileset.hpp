#pragma once
#include "editor.hpp"

class Tileset : public Scene {
public:
    void update(AppState* as) override;
    static void generate_cache(AppState* as, int track);
    std::string get_name() override;
};