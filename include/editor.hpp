#pragma once

#include <vector>
#include <SDL3/SDL.h>
#include <fstream>
#include <array>
#include <list>
#include "types.h"

class Scene;

typedef struct gameContext {
    uint8_t* eof;
    TrackTable* track_table;
    std::array<TrackDefinition*, TRACK_COUNT> definition_table;
    std::array<TrackHeader*, TRACK_COUNT> track_headers;
} GameContext;

typedef struct editorContext {
    SDL_AppResult app_result = SDL_APP_CONTINUE;
    int selected_track = -1;
    int selected_tile = -1;
    std::vector<uint8_t> file;
    bool file_open = false;
    std::vector<Scene*> scenes;

    std::array<SDL_Texture*,256> tile_buffer; 
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
    virtual std::string get_name() = 0;
    bool open;
};