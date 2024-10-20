#pragma once
#include <SDL3/SDL.h>
#include "types.h"

class Graphics {
public:
    static SDL_Surface* decode_4bpp(uint8_t* data, palette16 pal);
    static SDL_Surface* decode_8bpp(uint8_t* data, palette256 pal);
    static SDL_Color bgr_sdl(BGR col);
};