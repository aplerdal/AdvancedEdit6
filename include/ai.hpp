#pragma once

#include "editor.hpp"
#include "tools.hpp"

#define CIRCLE_RAD 5.0f
#define HOVER_BORDER_SIZE 3.0f

class AI : public Scene {
public:
    AI();
    void update(AppState* as) override;
    void DrawAILayout(AppState* as);
    void undo(AppState* as);
    void redo(AppState* as);
    void SectorInput(AppState* as, TrackContext* t);
    void SectorDraw(ImDrawList *dl, TrackContext *t);
    std::string get_name() override;

private:
    ViewTool* view;
    MapState state;

    int hovered_sector = -1;
    bool target_hovered = false;

    bool dragging = false;
    bool is_dragging_target = false;
    int drag_sector = -1;
    ImVec2 drag_start = ImVec2();
};

static void ZoneArms(uint8_t shape, ImVec2 vertex, float tri_size, ImVec2& armx, ImVec2& army);
static void DrawSector(ImDrawList* dl, MapState state, AiZone* zone, AiTarget* target);
static void DrawTarget(ImDrawList* dl, MapState state, AiZone* zone, AiTarget* t);

static bool PointInTriangle(ImVec2 point, ImVec2 vertex, uint8_t shape, float size);
static bool PointInCircle(ImVec2 point, ImVec2 position, float radius);
static bool PointInRect(ImVec2 point, ImVec2 min, ImVec2 max);

class TranslateCmd : public Command {
public:
    TranslateCmd(AppState* as, int drag_sector, bool drag_target, ImVec2 old_pos, ImVec2 new_pos);
    void execute(AppState* as) override;
    void redo(AppState* as) override;
    void undo(AppState* as) override;
private:
    ImVec2 old_pos;
    ImVec2 new_pos;
    int drag_sector;
    bool dragging_target;
};