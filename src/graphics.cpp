#include "graphics.hpp"

#include <SDL3/SDL.h>

#include <cstring>

#include "types.h"


SDL_Surface* Graphics::decode_4bpp(uint8_t* data, SDL_Palette* pal)
{
    // TODO implement this
    // SDL_Surface* buf = SDL_CreateSurface(8,8, SDL_PIXELFORMAT_XBGR1555);
    // BGR col_buf[64];
    // for (int i = 0; i < 32; i++) {
    //     col_buf[i*2]   = pal[data[i]&0xf0>>8];
    //     col_buf[i*2+1] = pal[data[i]&0xf];
    // }
    // if (buf->pixels != NULL){
    //     std::memcpy(buf->pixels, &col_buf, sizeof(col_buf));
    // }
    return nullptr;
}
SDL_Surface* Graphics::decode_8bpp(uint8_t* data, SDL_Palette* pal)
{
    SDL_Surface* buf = SDL_CreateSurface(8,8, SDL_PIXELFORMAT_INDEX8);
    SDL_SetSurfacePalette(buf, pal);
    if (buf->pixels != NULL){
        std::memcpy(buf->pixels, data, sizeof(uint8_t)*64);
    }
    return buf;
}