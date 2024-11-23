#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <SDL3/SDL.h>


#include <vector>
#include <fstream>
#include <array>
#include <deque>
#include <list>
#include <stack>
#include "types.h"

class Command;

typedef struct _vec2i vec2i;

typedef struct _vec2i {
    int32_t x, y;

    bool operator==(const vec2i& v) const {
        return x == v.x && y == v.y;
    }
} vec2i;

namespace std {
    template <>
    struct hash<vec2i> {
        std::size_t operator()(const vec2i& v) const {
            // Combine x and y into a single hash value
            return std::hash<uint64_t>()(static_cast<uint64_t>(v.x) << 32) | (static_cast<uint32_t>(v.y));
        }
    };
}

typedef struct _vec2{
    float x;
    float y;
} vec2;

class Scene;

typedef struct track {
    track_definition_t* definition_table;
    track_header_t* track_header;
    ai_header_t* ai_header;
    std::vector<ai_zone_t*> ai_zones;
    std::array<std::vector<ai_target_t*>,3> ai_targets;
} TrackContext;

typedef struct gameContext {
    uint8_t* eof;
    int track_width, track_height;
    TrackTable* track_table;
    std::array<TrackContext, TRACK_COUNT> tracks;
} GameContext;

typedef struct editorContext {
    float scroll_wheel;
    SDL_AppResult app_result = SDL_APP_CONTINUE;
    int selected_track = -1;
    int selected_tile = -1;
    std::vector<uint8_t> file;
    bool file_open = false;
    std::string file_name;
    std::vector<Scene*> scenes;
    Scene* inspector;

    std::deque<Command*> undo_stack;
    std::deque<Command*> redo_stack;

    SDL_Palette* palette;
    SDL_Texture* tile_buffer;
    SDL_Texture* map_buffer;
    std::vector<uint8_t> layout_buffer;
} EditorContext;

typedef struct appState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint64 last_step;
    EditorContext editor_ctx;
    GameContext game_ctx;
} AppState;

static const struct
{
    const char *key;
    const char *value;
} extended_metadata[] =
{
    { SDL_PROP_APP_METADATA_URL_STRING, "https://github.com/aplerdal" },
    { SDL_PROP_APP_METADATA_CREATOR_STRING, "Aplerdal" },
    { SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "Copyright 2024 Andrew \"Aplerdal\" Lerdal" },
    { SDL_PROP_APP_METADATA_TYPE_STRING, "program" }
};
class Scene {
public:
    virtual void update(AppState* as) = 0; 
    virtual void inspector(AppState* as) {
        ImGui::Text("No Window Focused");
    };
    virtual std::string get_name() = 0;
    bool open = true;
};

class Command {
public:
    virtual void execute(AppState* as) = 0;
    virtual void redo(AppState* as)= 0;
    virtual void undo(AppState* as)= 0;
};

inline void PUSH_STACK(std::deque<Command*>& queue, Command* value) {
    queue.push_back(value);
    if (queue.size()>32){
        delete queue.front();
        queue.pop_front();
    }
}