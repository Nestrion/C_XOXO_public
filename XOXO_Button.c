#include "include/XOXO_Button.h"

bool XOXO_ButtonTileClicked(SDL_Rect *button_rect)
{
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    if (mouse_x > button_rect->x &&
        mouse_x < button_rect->x +
                  button_rect->w &&
        mouse_y > button_rect->y &&
        mouse_y < button_rect->y +
                  button_rect->h)
        {
            return true;
        }
    else
    {
        return false;
    }
}