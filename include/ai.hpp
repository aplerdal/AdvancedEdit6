#pragma once

#include "editor.hpp"
#include "tools.hpp"

#define SEL_DIST 5.0f
#define HOVER_BORDER_SIZE 3.0f

// Organized by Priority, highest index is highest priority.
enum SectorPart{
    SECTOR_PART_NONE,
    SECTOR_PART_ZONE,
    SECTOR_PART_SCALE_HYPOT,
    SECTOR_PART_SCALE_ANGLE,
    SECTOR_PART_SCALE_N,
    SECTOR_PART_SCALE_S,
    SECTOR_PART_SCALE_E,
    SECTOR_PART_SCALE_W,
    SECTOR_PART_SCALE_NW,
    SECTOR_PART_SCALE_SW,
    SECTOR_PART_SCALE_NE,
    SECTOR_PART_SCALE_SE,
    SECTOR_PART_TARGET,
};
enum ScalePart{
    SCALE_NESW,
    SCALE_NWSE,
    SCALE_NS,
    SCALE_EW
};

class AI : public Scene {
public:
    AI();
    void update(AppState* as) override;
    void inspector(AppState* as) override;
    void DrawLayout(AppState* as);
    void undo(AppState* as);
    void redo(AppState* as);
    void HandleInput(AppState*as, TrackContext* t);
    void SectorDraw(ImDrawList *dl, TrackContext *t);
    void BeginDrag(int sector, SectorPart part, AiZone old_zone, AiTarget old_target, ImVec2 offset);
    std::string get_name() override;

private:
    ViewTool* view;
    MapState state;

    int hovered_sector = -1;
    SectorPart hover_part = SECTOR_PART_NONE;
    
    int selected_sector = -1;
    SectorPart selected_part = SECTOR_PART_NONE;

    bool dragging = false;
    SectorPart drag_part = SECTOR_PART_NONE;
    int drag_sector = -1;
    AiZone old_zone;
    AiTarget old_target;
    ImVec2 drag_offset = ImVec2();
};

static void GetZonePoints(AiZone* zone, ImVec2& vertex, ImVec2& armx, ImVec2& army);
static void DrawSector(ImDrawList* dl, MapState state, AiZone* zone, AiTarget* target);
static void DrawTarget(ImDrawList* dl, MapState state, AiZone* zone, AiTarget* t);

static bool PointInTriangle(ImVec2 point, ImVec2 vertex, ImVec2 armx, ImVec2 army);
static bool PointInCircle(ImVec2 point, ImVec2 position, float radius);
static bool PointInRect(ImVec2 point, ImVec2 min, ImVec2 max);
static bool PointInDiag(ImVec2 point, ImVec2 vertex, ImVec2 armx, ImVec2 army, float radius);

template<typename T> T clampCast(float value);

class AiModifyCmd : public Command {
public:
    AiModifyCmd(AppState* as, int drag_sector, AiZone old_zone, AiZone new_zone, AiTarget old_target, AiTarget new_target);
    void execute(AppState* as) override;
    void redo(AppState* as) override;
    void undo(AppState* as) override;
private:
    AiZone old_zone;
    AiZone new_zone;
    AiTarget old_target;
    AiTarget new_target;
    int drag_sector;
};