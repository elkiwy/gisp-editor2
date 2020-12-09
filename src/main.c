//Include SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

//Include standard libraries
#include <string.h>
#include <stdio.h>

//Include other soruce files
#include "KSDL_Text.h"
#include "KSDL_Cursor.h"

//Define constants
#define WINDOW_W 1280
#define WINDOW_H 720
#define BUFFER_SIZE 1024*10

//Declare global variables
SDL_Renderer* gRenderer;
SDL_Window* gWindow;
TTF_Font* gFont;

//Text area
KSDL_Text* textArea;
char textBuffer[BUFFER_SIZE];

//Cursror
KSDL_Cursor* cursor;

//Utilities
KSDL_Text* debugText;
void dLog(char* t){printf("\n%s", t);fflush(stdout);}
void dLogInt(char* s, int n){printf("\n%s: %d", s, n);fflush(stdout);}
void dLogPtr(void* p){printf("\n%p", p);fflush(stdout);}




/**
 *
 * DEBUG
 *
 * */
void updateDebugText(){
    sprintf(debugText->text, "pos:%d, line: %d, col: %d, currentLine: '%s', lineStart: %d, lineEnd: %d",
            cursor->pos, cursor->line, cursor->col, cursor->lineText, cursor->lineStart, cursor->lineEnd);
    KSDL_updateText(debugText);
}



/**
 *
 * Utilities
 *
 * */
void insertCharAtPos(char* b, char c, unsigned int pos){
    int size_after = strlen(b)-pos;
    char tempBuff[BUFFER_SIZE] = {};
    strncpy(tempBuff, &b[pos], size_after);
    b[pos] = c;
    strncpy(&b[pos+1], tempBuff, size_after);
}

void deleteCharAtPos(char* b, unsigned int pos){
    int size = strlen(b);
    int size_after = size-pos;
    char tempBuff[BUFFER_SIZE] = {};
    strncpy(tempBuff, &b[pos], size_after);
    strncpy(&b[pos-1], tempBuff, size_after);
    b[size-1] = '\0';
}

void deleteCharsFromTo(int a, int b){
        dLogInt("deleting a", a);
        dLogInt("deleting b", b);
    int start = (a < b) ? a : b;
    int end = (a < b) ? b : a;
    for (int i=start; i<end; i++){
        dLogInt("Deleting char at", i);
        deleteCharAtPos(textBuffer, start+1);
    }
}


/**
 *
 * Shortcuts
 *
 * */
void resetSelection(){
    cursor->selectionStart = -1;
}


void deleteBeforeCursor(){
    if(cursor->selectionStart != -1){
        //Delete block
        deleteCharsFromTo(cursor->selectionStart, cursor->pos);

        //If start is greater then pos i'm ok, else move cursor back to start
        while (cursor->selectionStart < cursor->pos){
            KSDL_moveCursor(cursor, -1, 0);
        }
    }else{
        //Delete only one char
        deleteCharAtPos(textArea->text, cursor->pos);
        KSDL_moveCursor(cursor, -1, 0);
    }
    //Update rendering
    KSDL_updateText(textArea);
    updateDebugText();
    resetSelection();
}

void deleteAfterCursor(){
    //For selection deleting, act like backspace
    if(cursor->selectionStart != -1){
        deleteBeforeCursor();
    //Else delete after
    }else{
        deleteCharAtPos(textArea->text, cursor->pos+1);
        KSDL_updateText(textArea);
        updateDebugText();
        resetSelection();
    }
}

void updateTextAreaWithChar(char c){
    //Delete first if i'm selecting
    if(cursor->selectionStart != -1){deleteBeforeCursor();}

    //Update the text area
    insertCharAtPos(textBuffer, c, cursor->pos);
    KSDL_moveCursor(cursor, 1, 0);
    KSDL_updateText(textArea);
    updateDebugText();
}



int safeAreaTop = WINDOW_H*0.1;
int safeAreaBot = WINDOW_H*0.8;
void moveCursor(int dx, int dy, int shiftDown){
    //Handle selection
    if (shiftDown && cursor->selectionStart == -1){
        cursor->selectionStart = cursor->pos;
    }else if (!shiftDown && cursor->selectionStart != -1){
        cursor->selectionStart = -1;
    }

    //Update cursor position
    KSDL_moveCursor(cursor, dx, dy);

    //Update scroll position
    int lineHeight = cursor->rect.h;
    int cursorPos = cursor->line * lineHeight;
    int thresholdLow = textArea->scrollY + safeAreaBot;
    int thresholdHigh = textArea->scrollY + safeAreaTop;

    //Scroll up if needed
    while(cursorPos < thresholdHigh){
        textArea->scrollY -= lineHeight;
        thresholdHigh = textArea->scrollY + safeAreaTop;
    }

    //Scroll down if needed
    while(cursorPos > thresholdLow){
        textArea->scrollY += lineHeight;
        thresholdLow = textArea->scrollY + safeAreaBot;
    }
}



/**
 *
 * Input/Output
 *
 * */
void readFileToBuffer(char* path, char* buffer){
    FILE* f = fopen(path, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(buffer, 1, fsize, f);
    fclose(f);
    buffer[fsize] = '\0';
}

void writeBufferToFile(char* path, char* buffer){
    FILE* f = fopen(path, "w+");
    fputs(buffer, f);
    fclose(f);
    dLog("File written!");
}







/**
 *
 * Main
 *
 * */
int main(int argc, char** argv) {
    //Initialize SDL, Window, Renderer, and Fonts
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_W, WINDOW_H, 0, &gWindow, &gRenderer);
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0);
    TTF_Init();
    gFont = TTF_OpenFont("../fonts/FiraCode-Regular.ttf", 14);
    if (gFont == NULL) {fprintf(stderr, "error: font not found\n"); return 1;}

    //Debug text
    char debugBuffer[1024] = "";
    debugText = KSDL_initText(gRenderer, debugBuffer, 0, WINDOW_H-100, WINDOW_W, 100, gFont);

    //Check for input
    char* path;
    if (argc>1){path = argv[1]; readFileToBuffer(path, textBuffer);
    }else{textBuffer[0] = '\0';}

    //Initialize cursor
    cursor = KSDL_initCursor(gRenderer, textBuffer, BUFFER_SIZE, gFont);

    //Initialize text area
    textArea = KSDL_initText(gRenderer, &textBuffer[0], 0, 0, WINDOW_W, WINDOW_H, gFont);
    KSDL_updateText(textArea);
    moveCursor(0, 0, 0);

    //Main Loop
    SDL_StartTextInput();
    SDL_Event event;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&event) == 1) {
            switch(event.type){
                //Quit event
                case SDL_QUIT:
                    quit = 1;
                    break;

                //TextInput event
                case SDL_TEXTINPUT:
                    updateTextAreaWithChar(event.text.text[0]);
                    break;

                //Other special Keydown event not intercepted by textInput
                case SDL_KEYDOWN:

                    if (event.key.keysym.mod & KMOD_GUI){
                        switch(event.key.keysym.sym){
                            case SDLK_s: writeBufferToFile(path, textBuffer); break;
                        }

                    }else if (event.key.keysym.mod & KMOD_SHIFT){
                        switch(event.key.keysym.sym){
                            case SDLK_LEFT:    moveCursor(-1,  0, 1); break;
                            case SDLK_RIGHT:   moveCursor( 1,  0, 1); break;
                            case SDLK_UP:      moveCursor( 0, -1, 1); break;
                            case SDLK_DOWN:    moveCursor( 0,  1, 1); break;
                        }
                    }else{
                        switch(event.key.keysym.sym){
                            case SDLK_ESCAPE:    quit = 1; break;
                            case SDLK_BACKSPACE: deleteBeforeCursor(); break;
                            case SDLK_DELETE:    deleteAfterCursor(); break;
                            case SDLK_RETURN:    updateTextAreaWithChar('\n'); break;
                            case SDLK_LEFT:      moveCursor(-1,  0, 0); break;
                            case SDLK_RIGHT:     moveCursor( 1,  0, 0); break;
                            case SDLK_UP:        moveCursor( 0, -1, 0); break;
                            case SDLK_DOWN:      moveCursor( 0,  1, 0); break;
                            default: dLog("Key not recognized:");dLog((char *)SDL_GetKeyName(event.key.keysym.sym));
                        }
                    }
                    updateDebugText();
            }
        }



        //Clear the screen
        SDL_RenderClear(gRenderer);

        //Render text area
        KSDL_drawText(textArea);
        //KSDL_drawText(debugText);
        KSDL_drawCursor(cursor, textArea->scrollX, textArea->scrollY);

        //Render all the Renderer informations into the window
        SDL_RenderPresent(gRenderer);

    }

    //Free all the objects
    KSDL_freeCursor(cursor);
    KSDL_freeText(textArea);
    KSDL_freeText(debugText);
    TTF_Quit();
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
    return 0;
}
