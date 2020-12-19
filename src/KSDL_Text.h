#ifndef __KSDL_TEXT_H_
#define __KSDL_TEXT_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef struct KSDL_Text{
    char* text;
    SDL_Texture* texture;
    SDL_Rect rect;
    SDL_Color fgColor;
    SDL_Color bgColor;
    TTF_Font* font;
    SDL_Renderer* renderer;
    int maxWidth;
    int maxHeight;

    int scrollX;
    int scrollY;

    SDL_Rect backgroundRect;
    SDL_Color backgroundColor;
}KSDL_Text;

KSDL_Text* KSDL_initText(SDL_Renderer* r, char* text, int x, int y, int w, int h, TTF_Font* f);
void KSDL_updateText(KSDL_Text* t);
void KSDL_freeText(KSDL_Text* t);
void KSDL_drawText(KSDL_Text* t);


#endif // __KSDL_TEXT_H_
