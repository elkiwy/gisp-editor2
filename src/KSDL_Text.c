#include "KSDL_Text.h"

KSDL_Text* KSDL_initText(SDL_Renderer* r, char* text, int x, int y, int w, int h, TTF_Font* f){
    //Init object
    KSDL_Text* t = malloc(sizeof(KSDL_Text));
    t->text = text;
    t->font = f;
    t->renderer = r;

    //Init colors
    t->fgColor.r = 0xff; t->fgColor.g = 0xff; t->fgColor.b = 0xff; t->fgColor.a = 0xff;

    //Setup rect
    t->rect.x = x; t->rect.y = y; t->rect.w = 0; t->rect.h = 0;
    t->maxWidth = w; t->maxHeight = h;
    t->scrollX = 0;
    t->scrollY = 0;

    KSDL_updateText(t);

    return t;
}

void KSDL_updateText(KSDL_Text* t){


    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(t->font, t->text, t->fgColor, t->maxWidth);
    if (strlen(t->text) == 0){surface = TTF_RenderText_Blended_Wrapped(t->font, " ", t->fgColor, t->maxWidth);}
    t->texture = SDL_CreateTextureFromSurface(t->renderer, surface);
    t->rect.w = surface->w;
    t->rect.h = surface->h;
    SDL_FreeSurface(surface);
}


void KSDL_freeText(KSDL_Text* t){
    SDL_DestroyTexture(t->texture);
    free(t);
}

void KSDL_drawText(KSDL_Text* t){
    SDL_Rect scrolledRect = {t->rect.x - t->scrollX, t->rect.y - t->scrollY, t->rect.w, t->rect.h};
    SDL_RenderCopy(t->renderer, t->texture, NULL, &scrolledRect);
}
