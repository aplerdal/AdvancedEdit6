#pragma once

#include <SDL3/SDL.h>
#include "editor.hpp"

static const SDL_DialogFileFilter gbaFileFilter[] = {
    { "GBA Roms",  "gba" },
    { "All files",   "*" }
};
static void SDLCALL OpenFileCallback(void* userdata, const char* const* filelist, int filter);

class MenuBar : public Scene {
public:
    void update(AppState* as) override;
    std::string get_name() override;
private:
    bool demo_open;
};