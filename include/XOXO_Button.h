#ifndef XOXO_BUTTON_H
#define XOXO_BUTTON_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>


typedef struct XOXO_ButtonTile XOXO_ButtonTile;
struct XOXO_ButtonTile
{
    SDL_Color XOXO_btn_color;
    SDL_Color XOXO_btn_text_color;
    SDL_Rect XOXO_btn_text_rect;
    SDL_Rect XOXO_btn_rect;
    TTF_Font *XOXO_btn_font;
    SDL_Surface *XOXO_btn_surface;
    SDL_Texture *XOXO_btn_texture;
};


bool XOXO_ButtonTileClicked(SDL_Rect *button_rect);

#endif