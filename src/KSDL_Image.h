#ifndef __KSDL_IMAGE_H_
#define __KSDL_IMAGE_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

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
    i->rect.h = h;

    KSDL_updateImage(i);

    i->backgroundColor.r = 0x00;
    i->backgroundColor.g = 0x00;
    i->backgroundColor.b = 0x00;
    i->backgroundColor.a = 0x00;
    i->backgroundRect.x = x;
    i->backgroundRect.y = y;
    i->backgroundRect.w = w;
    i->backgroundRect.h = h;
    return i;
}


void KSDL_freeImage(KSDL_Image* i){
    SDL_DestroyTexture(i->texture);
    free(i);
}

void KSDL_updateImage(KSDL_Image* i){
    //Read from file and create original surface
    SDL_RWops* rwop = SDL_RWFromFile(i->imagePath, "rb");
    SDL_Surface* surface = IMG_LoadPNG_RW(rwop);
    if(surface == NULL){printf("\n!!! Can't load image '%s'.\n", i->imagePath); return;}

    //Define target dimensions
    int targetSize = (surface->w < i->rect.w) ? surface->w : i->rect.w;
    SDL_Rect targetDimensions = {0,0,targetSize,targetSize,};

    //Create a 32BPP version of the original surface
    SDL_Surface* p32BPPSurface = SDL_CreateRGBSurface(surface->flags, surface->w, surface->h, 32, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
    if(SDL_BlitSurface(surface, NULL, p32BPPSurface, NULL) < 0){printf("Error did not blit surface %s\n", SDL_GetError());return;}

    //Scale the 32BPP version
    SDL_Surface* scaledSurface = SDL_CreateRGBSurface(p32BPPSurface->flags, targetSize, targetSize, p32BPPSurface->format->BitsPerPixel, p32BPPSurface->format->Rmask, p32BPPSurface->format->Gmask, p32BPPSurface->format->Bmask, p32BPPSurface->format->Amask);
    SDL_FillRect(scaledSurface, &targetDimensions, SDL_MapRGBA(scaledSurface->format, 255, 0, 0, 255));
    if (SDL_BlitScaled(p32BPPSurface, NULL, scaledSurface, NULL) < 0){printf("Error did not scale surface: %s\n", SDL_GetError()); SDL_FreeSurface(scaledSurface);return;}

    //Replace the original surface with the scaled one
    SDL_FreeSurface(surface);
    surface = scaledSurface;

    //Get the resized dimension (should be == to the target dimensions)
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
