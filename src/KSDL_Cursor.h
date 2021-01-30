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
    TTF_Font* font;
    float charOffset;

    //Line delimiters
    unsigned int lineStart;
    unsigned int lineEnd;

    //Line and column
    unsigned int line;
    unsigned int col;

    //Line buffer
    char* lineText;

    //Selection
    int selectionStart;


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


    SDL_Surface* s = TTF_RenderText_Blended(f, "o", color);
    c->texture = SDL_CreateTextureFromSurface(r, s);
    c->charOffset = -0.5;
    c->rect.x = s->w * c->charOffset;
    c->rect.y = 0;
    c->rect.w = s->w;
    c->font = f;
    SDL_Surface* fakeSymbolForHeight = TTF_RenderText_Blended(f, "o", color);
    c->rect.h = fakeSymbolForHeight->h;
    SDL_FreeSurface(fakeSymbolForHeight);

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

    //Selection
    c->selectionStart = -1;

    return c;
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
            nextNLPos = i;
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
    int maxPos = strlen(c->buffer)-1;
    if (dx!=0){
        //Update cursors position for LEFT and RIGHT
        c->pos += dx;

    }else if(dy!=0){
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
    c->rect.x = c->col * c->rect.w - (c->rect.w)*c->charOffset;
    c->rect.y = c->line * c->rect.h;
}


void KSDL_changeCursor(KSDL_Cursor* c, char symbol, float charOffset){
    SDL_Color color = {0xff,0xff,0xff,0xff};
    char symbolStr[2]; symbolStr[0] = symbol; symbolStr[1] = '\0';
    SDL_Surface* s = TTF_RenderText_Blended(c->font, symbolStr, color);
    c->texture = SDL_CreateTextureFromSurface(c->renderer, s);
    c->charOffset = -charOffset;
    c->rect.x = s->w * c->charOffset;
    c->rect.y = 0;
    c->rect.w = s->w;
    KSDL_moveCursor(c, 0, 0);
}




///Deinitialize cursor
void KSDL_freeCursor(KSDL_Cursor* c){
    free(c->lineText);
    free(c);
}



void positionToLineAndColumn(char* buffer, int pos, int* line, int* col){
    int l = 0; int c = 0; int i = 0;
    while(i < pos){
        if (buffer[i]=='\n'){
            c = 0;
            l++;
        }else{
            c++;
        }
        i++;
    }
    *line = l; *col = c;
}


int KSDL_getLineStart(KSDL_Cursor* c){
    int i = c->pos;
    while(i > 0){
        char chr = c->buffer[i];
        if (chr == '\n'){
            return i+1;
        }
        i--;
    }
    return 0;
}

int KSDL_getLineEnd(KSDL_Cursor* c){
    int i = c->pos;
    while(i < c->bufferLen-1){
        char chr = c->buffer[i];
        if (chr == '\n'){
            return i-1;
        }
        i++;
    }
    return c->bufferLen-1;
}









void KSDL_drawCursor(KSDL_Cursor* c, int scrollX, int scrollY){
    SDL_Rect scrolledRect = {c->rect.x - scrollX, c->rect.y - scrollY, c->rect.w, c->rect.h};
    SDL_RenderCopy(c->renderer, c->texture, NULL, &scrolledRect);



    if (c->selectionStart != -1){
        /* setup */
        int a = (c->selectionStart < c->pos) ? c->selectionStart : c->pos;
        int b = (c->selectionStart < c->pos) ? c->pos : c->selectionStart;
        if (a==b){return;}

        char tmp[c->bufferLen];
        strncpy(tmp, &(c->buffer[a]), b-a);
        tmp[b-a] = '\0';


        /* line splitting */
        int linesPos[c->bufferLen];
        int linesCount = 0;
        linesPos[0] = 0;
        for(unsigned int i=0;i<strlen(tmp);++i){
            if (tmp[i]=='\n'){
                linesCount++;
                linesPos[linesCount] = i+1;
            }
        }
        linesPos[linesCount+1] = b-a;


        /* Rendering */
        SDL_Color fg = {0x00, 0x00, 0x00, 0xff};
        SDL_Color bg = {0xff, 0xff, 0xff, 0xff};
        int lineHeight = c->rect.h;
        for (int i=0;i<=linesCount;++i){

            /* Get the line and column of the first point */
            int tmp_col; int tmp_line;
            positionToLineAndColumn(c->buffer, a+linesPos[i], &tmp_line, &tmp_col);
            int tmp_x = tmp_col * c->rect.w;
            int tmp_y = tmp_line * c->rect.h;

            /* get current line text */
            int lineLength = linesPos[i+1] - linesPos[i];
            if (lineLength<=0){continue;}
            char tmpLine[lineLength+1];
            strncpy(tmpLine, &(tmp[linesPos[i]]), lineLength);
            tmpLine[lineLength] = '\0';

            if (tmpLine[lineLength-1] == '\n'){tmpLine[lineLength-1] = '\0';}

            /* render it */
            SDL_Surface* surface = TTF_RenderText_Shaded(c->font, tmpLine, fg, bg);
            if (surface==NULL){continue;}
            SDL_Texture* texture = SDL_CreateTextureFromSurface(c->renderer, surface);
            SDL_Rect r = {tmp_x - scrollX, tmp_y - scrollY, surface->w, surface->h};
            SDL_RenderCopy(c->renderer, texture, NULL, &r);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }

        //printf("Selection is %s", tmp);fflush(stdout);
    }
}



#endif // __KSDL_CURSOR_H_
