#pragma once

#include "editor.hpp"
#include <unordered_map>

typedef struct map_state{
    ImVec2 win_pos; 
    ImVec2 win_size;
    ImVec2 cursor_pos;
    ImVec2 track_size;
    ImVec2 translation = ImVec2(0.0f,0.0f);
    float scale = 1.0f;
} MapState;

class Tool {
public:
    virtual void update(AppState* as, MapState& ms) {};
};

class TilePickerCommand : public Tool {
    TilePickerCommand(vec2i position);
};

typedef std::unordered_map<vec2i, uint8_t> TileBuffer;

class DrawCmd : public Command {
public:
    DrawCmd(AppState* as, TileBuffer tile_buf);
    void execute(AppState* as) override;
    void redo(AppState* as) override;
    void undo(AppState* as) override;
private:
    TileBuffer old_tiles;
    TileBuffer new_tiles;

    SDL_FRect src;
    SDL_FRect dst;
};
class DrawTool : public Tool {
public:
    void update(AppState* as, MapState& ms) override;
private:
    bool held;
    TileBuffer draw_buf;
};

class BucketTool : public Tool {
public:
    void update(AppState* as, MapState& ms) override;
};

class ViewTool : public Tool {
public:
    void update(AppState* as, MapState& ms) override;
private:
    bool dragging = false;
    ImVec2 drag_pos = ImVec2();
    ImVec2 drag_map_pos = ImVec2();
};