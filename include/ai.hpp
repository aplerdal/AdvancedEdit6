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