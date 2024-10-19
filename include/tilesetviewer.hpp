#pragma once
#include "editor.hpp"

class TilesetViewer : public Scene {
public:
    void update(AppState* as) override;
    std::string get_name() override;
};