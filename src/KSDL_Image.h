#ifndef __KSDL_IMAGE_H_
#define __KSDL_IMAGE_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_rotozoom.h>

typedef struct KSDL_Image{
    char* imagePath;
    SDL_Texture* texture;
    SDL_Renderer* renderer;

    SDL_Rect rect;

    SDL_Rect backgroundRect;
    SDL_Color backgroundColor;
}KSDL_Image;


void KSDL_updateImage(KSDL_Image* i);



KSDL_Image* KSDL_initImage(SDL_Renderer* r, char* path, int x, int y, int w, int h){
    KSDL_Image* i = malloc(sizeof(KSDL_Image));
    i->imagePath = path;
    i->renderer = r;
    i->texture = NULL;
    i->rect.x = x;
    i->rect.y = y;
    i->rect.w = w;
    i->rect.h = w;

    i->backgroundColor.r = 0x00;
    i->backgroundColor.g = 0x00;
    i->backgroundColor.b = 0x00;
    i->backgroundColor.a = 0x00;
    i->backgroundRect.x = x;
    i->backgroundRect.y = y;
    i->backgroundRect.w = w;
    i->backgroundRect.h = h;

    KSDL_updateImage(i);

    return i;
}


void KSDL_freeImage(KSDL_Image* i){
    SDL_DestroyTexture(i->texture);
    free(i);
}

void KSDL_updateImage(KSDL_Image* i){
    //Read from file and create original surface
    SDL_Surface* surface = IMG_Load(i->imagePath);
    if(surface == NULL){printf("\n!!! Can't load image '%s'.\n", i->imagePath); return;}

    //Get the scale factor needed and scale if with SDL_gfx rotozoom lib
    float scale = (float)i->backgroundRect.w / (float)surface->w;
    SDL_Surface* zoomedSurface = zoomSurface(surface, scale, scale, SMOOTHING_ON);
    SDL_FreeSurface(surface);
    surface = zoomedSurface;
    i->rect.w = surface->w;
    i->rect.h = surface->h;

    //Convert to a texture
    i->texture = SDL_CreateTextureFromSurface(i->renderer, surface);
    if(i->texture == NULL) {printf("\n!!!Unable to create texture from %s! SDL Error: %s\n", i->imagePath, SDL_GetError());}
    SDL_FreeSurface(surface);
}



void KSDL_drawImage(KSDL_Image* i){
    //Read the current draw color
    Uint8 prevR, prevG, prevB, prevA;
    SDL_GetRenderDrawColor(i->renderer, &prevR, &prevG, &prevB, &prevA);

    //Draw the background
    SDL_SetRenderDrawColor(i->renderer, i->backgroundColor.r, i->backgroundColor.g, i->backgroundColor.b, i->backgroundColor.a);
    SDL_RenderFillRect(i->renderer, &(i->backgroundRect));

    //Reset draw state
    SDL_SetRenderDrawColor(i->renderer, prevR, prevG, prevB, prevA);

    //Draw the text
    if(i->texture != NULL){
        SDL_Rect scrolledRect = {i->rect.x, i->rect.y, i->rect.w, i->rect.h};
        SDL_RenderCopy(i->renderer, i->texture, NULL, &scrolledRect);
    }
}






#endif // __KSDL_IMAGE_H_
