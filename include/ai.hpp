#pragma once

#include "editor.hpp"
#include "tools.hpp"

#define SEL_DIST 5.0f
#define HOVER_BORDER_SIZE 3.0f

static const SDL_DialogFileFilter aiFileFilter[] = {
    { "Super Circuit AI File",  "scai" },
    { "Bin File",  "bin" },
    { "All files",   "*" }
};

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
    void HandleInput(AppState* as, TrackContext* t);
    void SectorDraw(ImDrawList* dl, TrackContext* t);
    void CreateSector(AppState* as);
    void BeginDrag(int sector, SectorPart part, ai_zone_t old_zone, ai_target_t old_target, ImVec2 offset);
    std::vector<uint8_t> Save(AppState* as);
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
    ai_zone_t old_zone;
    ai_target_t old_target;
    ImVec2 drag_offset = ImVec2();
};

static void SDLCALL SaveAIDialog(void* userdata, const char* const* filelist, int filter);

static void GetZonePoints(ai_zone_t* zone, ImVec2& vertex, ImVec2& armx, ImVec2& army);
static void DrawSector(ImDrawList* dl, MapState state, ai_zone_t* zone, ai_target_t* target);
static void DrawTarget(ImDrawList* dl, MapState state, ai_zone_t* zone, ai_target_t* t);

static bool PointInTriangle(ImVec2 point, ImVec2 vertex, ImVec2 armx, ImVec2 army);
static bool PointInCircle(ImVec2 point, ImVec2 position, float radius);
static bool PointInRect(ImVec2 point, ImVec2 min, ImVec2 max);
static bool PointInDiag(ImVec2 point, ImVec2 vertex, ImVec2 armx, ImVec2 army, float radius);

template<typename T> T clampCast(float value);

class AiModifyCmd : public Command {
public:
    AiModifyCmd(AppState* as, int drag_sector, ai_zone_t old_zone, ai_zone_t new_zone, ai_target_t old_target, ai_target_t new_target);
    void execute(AppState* as) override;
    void redo(AppState* as) override;
    void undo(AppState* as) override;
private:
    ai_zone_t old_zone;
    ai_zone_t new_zone;
    ai_target_t old_target;
    ai_target_t new_target;
    int drag_sector;
};
class CreateSectorCmd : public Command {
public:
    CreateSectorCmd(AppState* as, ai_zone_t* new_zone, ai_target_t* new_target);
    void execute(AppState* as) override;
    void redo(AppState* as) override;
    void undo(AppState* as) override;
private:
    ai_zone_t* new_zone;
    ai_target_t* new_target;
};