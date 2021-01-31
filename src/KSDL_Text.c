#include "KSDL_Text.h"

KSDL_Text* KSDL_initText(SDL_Renderer* r, char* text, int x, int y, int w, int h, TTF_Font* f){
    //Init object
    KSDL_Text* t = malloc(sizeof(KSDL_Text));
    t->text = text;
    t->font = f;
    t->renderer = r;
    t->texture = NULL;

    //Init colors
    t->fgColor.r = 0xff; t->fgColor.g = 0xff; t->fgColor.b = 0xff; t->fgColor.a = 0xff;

    //Setup rect
    t->rect.x = x; t->rect.y = y; t->rect.w = w; t->rect.h = h;
    t->maxWidth = w; t->maxHeight = h;
    t->scrollX = 0;
    t->scrollY = 0;

    //Background
    t->backgroundColor.r = 0x00;
    t->backgroundColor.g = 0x00;
    t->backgroundColor.b = 0x00;
    t->backgroundColor.a = 0x00;
    t->backgroundRect.x = x;
    t->backgroundRect.y = y;
    t->backgroundRect.w = w;
    t->backgroundRect.h = h;

    KSDL_updateText(t);

    return t;
}



void KSDL_updateText(KSDL_Text* t){
    //Clean old texture if any
    if(t->texture != NULL){SDL_DestroyTexture(t->texture);}

    //Create new texture
    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(t->font, t->text, t->fgColor, t->maxWidth);
    if (strlen(t->text) == 0){surface = TTF_RenderText_Blended_Wrapped(t->font, " ", t->fgColor, t->maxWidth);}
    t->texture = SDL_CreateTextureFromSurface(t->renderer, surface);
    t->texture_w = surface->w;
    t->texture_h = surface->h;
    SDL_FreeSurface(surface);
}


void KSDL_freeText(KSDL_Text* t){
    SDL_DestroyTexture(t->texture);
    free(t);
}

void KSDL_drawText(KSDL_Text* t){
    //Read the current draw color
    Uint8 prevR, prevG, prevB, prevA;
    SDL_GetRenderDrawColor(t->renderer, &prevR, &prevG, &prevB, &prevA);

    //Draw the background
    SDL_SetRenderDrawColor(t->renderer, t->backgroundColor.r, t->backgroundColor.g, t->backgroundColor.b, t->backgroundColor.a);
    SDL_RenderFillRect(t->renderer, &(t->backgroundRect));

    //Reset draw state
    SDL_SetRenderDrawColor(t->renderer, prevR, prevG, prevB, prevA);

    //Draw the text
    if (t->texture != NULL){
        int tw = (t->rect.w < t->texture_w) ? t->rect.w : t->texture_w;
        int th = (t->rect.h < t->texture_h) ? t->rect.h : t->texture_h;
        SDL_Rect src = {
            t->rect.x + t->scrollX,
            t->rect.y + t->scrollY,
            tw, th
        };
        SDL_Rect dest = {
            t->rect.x,
            t->rect.y,
            tw, th
        };
        SDL_RenderCopy(t->renderer, t->texture, &src, &dest);
    }
}
