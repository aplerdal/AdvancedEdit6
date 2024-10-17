#include <SDL3/SDL.h>
#include <cstring>
#include "types.h"
#include "graphics.hpp"

SDL_Surface* Graphics::decode_4bpp(char* data, palette16 pal)
{
    SDL_Surface* buf = SDL_CreateSurface(8,8, SDL_PIXELFORMAT_XBGR1555);
    BGR col_buf[64];
    for (int i = 0; i < 32; i++) {
        col_buf[i*2]   = pal[data[i]&0xf0>>8];
        col_buf[i*2+1] = pal[data[i]&0xf];
    }
    if (buf->pixels != NULL){
        std::memcpy(buf->pixels, &col_buf, sizeof(col_buf));
    }

}
SDL_Surface* Graphics::decode_8bpp(char* data, palette256 pal)
{
    SDL_Surface* buf = SDL_CreateSurface(8,8, SDL_PIXELFORMAT_XBGR1555);
    BGR col_buf[64];
    for (int i = 0; i < 64; i++) {
        col_buf[i] = pal[data[i]];
    }
    if (buf->pixels != NULL){
        std::memcpy(buf->pixels, &col_buf, sizeof(col_buf));
    }
    return buf;
}
SDL_Color Graphics::bgr_sdl(BGR col)
{
    return {(u8)(col.r<<3),(u8)(col.g<<3),(u8)(col.b<<3),0xff};
}