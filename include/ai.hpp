#pragma once

#include "editor.hpp"
#include "tools.hpp"

class AI : public Scene {
public:
    AI();
    void update(AppState* as) override;
    void draw_ai_layout(AppState* as);
    void undo(AppState* as);
    void redo(AppState* as);
    std::string get_name() override;
private:
    ViewTool* view;
    MapState state;
};

static bool PointInTriangle(ImVec2 point, ImVec2 vertex, uint8_t shape, float size);
static bool PointInCircle(ImVec2 point, ImVec2 position, float radius);
static bool PointInRect(ImVec2 point, ImVec2 min, ImVec2 max);