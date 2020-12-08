#ifndef __KSDL_CURSOR_H_
#define __KSDL_CURSOR_H_


#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>




typedef struct KSDL_Cursor{
    int pos;
    char* buffer;
    unsigned int bufferLen;

    //SDL stuff
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Rect rect;

    //Line delimiters
    unsigned int lineStart;
    unsigned int lineEnd;

    //Line and column
    unsigned int line;
    unsigned int col;

    //Line buffer
    char* lineText;


}KSDL_Cursor;



///Initialize and create a cursor
KSDL_Cursor* KSDL_initCursor(SDL_Renderer* r, char* buffer, unsigned int len, TTF_Font* f){
    KSDL_Cursor* c = malloc(sizeof(KSDL_Cursor));
    c->buffer = buffer;
    c->bufferLen = len;
    c->pos = 0;

    //SDL stuff
    c->renderer = r;
    SDL_Color color = {0xff,0xff,0xff,0xff};
    SDL_Surface* s = TTF_RenderText_Blended(f, "|", color);
    c->texture = SDL_CreateTextureFromSurface(r, s);
    c->rect.x = s->w * -0.5;
    c->rect.y = 0;
    c->rect.w = s->w;
    c->rect.h = s->h-1;

    //Line delimiters
    c->lineStart = 0;
    char* nextNL = strchr(buffer, '\n');
    c->lineEnd = (nextNL == NULL) ? strlen(buffer) : nextNL - buffer;

    //Line and column
    c->line = 0;
    c->col = 0;

    //Line buffer
    char* lineBuffer = malloc(sizeof(char)*len);
    strncpy(lineBuffer, buffer, c->lineEnd);
    lineBuffer[c->lineEnd] = '\0';
    c->lineText = lineBuffer;

    return c;
}




///Deinitialize cursor
void KSDL_freeCursor(KSDL_Cursor* c){
    free(c->lineText);
    free(c);
}



///Updates the current line information of the cursor
void KSDL_updateCurrentLine(KSDL_Cursor* c){
    //Cycle the buffer for data
    int i = 0;
    unsigned int NLCount = 0;
    unsigned int prevNLPos = 0;
    unsigned int nextNLPos = 0;
    while(i < (int)c->bufferLen){
        if (c->buffer[i]=='\n'){
            if (i>=c->pos){
                nextNLPos = i;
                break;
            }else{
                NLCount++;
                prevNLPos = i+1;
            }
        }else if (c->buffer[i]=='\0'){
            nextNLPos = i-1;
            break;
        }
        i++;
    }


    if(strlen(c->buffer)==0){
        c->lineStart = 0;
        c->lineEnd = 0;
        c->line = 0;
        c->lineText[0] = '\0';
    }else{
        ////Copy from the occurrence before to the next
        strncpy(c->lineText, &c->buffer[prevNLPos], nextNLPos - prevNLPos);
        c->lineText[nextNLPos-prevNLPos] = '\0';

        //Update the line delimiters too
        c->lineStart = prevNLPos;
        c->lineEnd   = nextNLPos;
        c->line      = NLCount;
    }
}


void KSDL_moveCursor(KSDL_Cursor* c, int dx, int dy){
    int maxPos = strlen(c->buffer);
    if (dx!=0){
        //Update cursors position for LEFT and RIGHT
        c->pos += dx;

    }else{
        //Move to the start of the next line
        if(dy==1){
            //Go to next line
            c->pos += (c->lineEnd - c->pos)+1;

            //Update the current line and reapply the previous column offset
            KSDL_updateCurrentLine(c);
            unsigned int newLineLenght = strlen(c->lineText);
            c->pos += (c->col < newLineLenght) ? c->col : newLineLenght;
        }else{
            //Go to prev line start
            c->pos -= (c->col+1);

            //Update current line and reapply the previous column offset
            KSDL_updateCurrentLine(c);
            c->pos = (c->col < strlen(c->lineText)) ? c->lineStart + c->col : c->lineEnd;
        }
    }

    //Make sure to be in the correct space
    if (c->pos < 0){c->pos = 0;}
    else if(c->pos > maxPos){c->pos = maxPos;}
    else{c->pos = c->pos;}


    //Retrieve current line
    KSDL_updateCurrentLine(c);
    c->col = c->pos - c->lineStart;

    //Update cursor position on screen
    c->rect.x = c->col * c->rect.w - (c->rect.w)*0.5;
    c->rect.y = c->line * c->rect.h;
}


void KSDL_drawCursor(KSDL_Cursor* c, int scrollX, int scrollY){
    SDL_Rect scrolledRect = {c->rect.x - scrollX, c->rect.y - scrollY, c->rect.w, c->rect.h};
    SDL_RenderCopy(c->renderer, c->texture, NULL, &scrolledRect);
}



#endif // __KSDL_CURSOR_H_
