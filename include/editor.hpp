#pragma once

#include <vector>
#include <SDL3/SDL.h>
#include <fstream>

class Scene;

typedef struct {
    SDL_AppResult app_result = SDL_APP_CONTINUE;
    std::ifstream file;
    std::vector<Scene*> active_scenes;
} EditorContext;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint64 last_step;
    EditorContext editor_ctx;
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
    virtual ~Scene() = default;

    virtual void init(AppState* as) = 0;
    virtual void update(AppState* as, int id) = 0;
    virtual void exit(AppState* as) = 0;
};