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
    void init(AppState* as) override;
    void update(AppState* as, int id) override;
    void exit(AppState* as) override;
private:
    
};