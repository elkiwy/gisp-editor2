//Include SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

//Include standard libraries
#include <string.h>
#include <stdio.h>
#include <pthread.h>

//Include other soruce files
#include "KSDL_Text.h"
#include "KSDL_Cursor.h"
#include "KSDL_Image.h"

//Define constants
#define WINDOW_W 1920*0.5
#define WINDOW_H 1080*0.5
#define BUFFER_SIZE 1024*10

//Declare global variables
SDL_Renderer* gRenderer;
SDL_Window* gWindow;
TTF_Font* gFont;
int dpiScale = 1;

//Text area
KSDL_Text* textArea;
char textBuffer[BUFFER_SIZE];
typedef enum {INPUT_SIMPLE, INPUT_VIM} InputMode;
InputMode inputMode = INPUT_VIM;
typedef enum {VIM_NORMAL, VIM_INSERT, VIM_REPLACE} VimMode;
typedef enum {VIM_NONE, VIM_CHANGE, VIM_DELETE} VimSubMode;
VimMode vimMode = VIM_NORMAL;
VimSubMode vimSubMode = VIM_NONE;

//Console
KSDL_Text* consolePanel;

//Preview
KSDL_Image* outputPreview;

//Cursror
KSDL_Cursor* cursor;

//Utilities
KSDL_Text* debugText;
void dLog(char* t){printf("%s\n", t);fflush(stdout);}
void dLogInt(char* s, int n){printf("%s: %d\n", s, n);fflush(stdout);}
void dLogPtr(void* p){printf("%p\n", p);fflush(stdout);}




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
 * Commands
 *
 * */
//Define the possible commands operations
typedef enum {INS, DEL} CommandOperation;

//Define the struct for each command
typedef struct Command{
    char c;
    int pos;
    CommandOperation op;
}Command;

//Initialize the commands stack and stackpointer
Command commandsStack[1024];
int commandsStackPtr = 0;
int executingCommand = 0;

///Push a new command to the commands stack
void pushCommand(char c, int pos, CommandOperation op){
    //Make new object
    commandsStack[commandsStackPtr].c = c;
    commandsStack[commandsStackPtr].pos = pos;
    commandsStack[commandsStackPtr].op = op;

    //Push into the stack
    commandsStackPtr++;
}


void undo();





/**
 *
 * Utilities
 *
 * */
void insertCharAtPos(char* b, char c, unsigned int pos){
    //Insert the new character
    int size_after = strlen(b)-pos;
    char tempBuff[BUFFER_SIZE] = {};
    strncpy(tempBuff, &b[pos], size_after);
    b[pos] = c;
    strncpy(&b[pos+1], tempBuff, size_after);

    //Push a new command
    if(executingCommand==0){pushCommand(c, pos, INS);}
}

void deleteCharAtPos(char* b, unsigned int pos){
    //Delete the char
    int size = strlen(b);
    int size_after = size-pos;
    char c = b[pos-1];
    char tempBuff[BUFFER_SIZE] = {};
    strncpy(tempBuff, &b[pos], size_after);
    strncpy(&b[pos-1], tempBuff, size_after);
    b[size-1] = '\0';

    //Push a new command
    if(executingCommand==0){pushCommand(c, pos, DEL);}
}

void deleteCharsFromTo(int a, int b){
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



void moveCursor(int dx, int dy, int shiftDown){
    //Handle selection
    if (shiftDown && cursor->selectionStart == -1){
        cursor->selectionStart = cursor->pos;
    }else if (!shiftDown && cursor->selectionStart != -1){
        cursor->selectionStart = -1;
    }

    //Update cursor position
    KSDL_moveCursor(cursor, dx, dy);
    int lineHeight = cursor->rect.h;
    int cursorPos = cursor->line * lineHeight;

    //Set margins
    int safeAreaTop = 0;
    int safeAreaBot = textArea->backgroundRect.h - lineHeight;

    //Scroll up if needed
    int thresholdHigh = textArea->scrollY + safeAreaTop;
    while(cursorPos < thresholdHigh){
        textArea->scrollY -= lineHeight;
        thresholdHigh = textArea->scrollY + safeAreaTop;
    }

    //Scroll down if needed
    int thresholdLow = textArea->scrollY + safeAreaBot;
    while(cursorPos > thresholdLow){
        textArea->scrollY += lineHeight;
        thresholdLow = textArea->scrollY + safeAreaBot;
    }
}


void moveCursorAbsolute(int pos){
    cursor->pos = pos;
    KSDL_moveCursor(cursor, 0, 0);
}






/**
 *
 * Undo
 *
 * */

//Execute one undo operation
void undo(){
    if (commandsStackPtr<=0){return;}
    executingCommand = 1;
    Command c = commandsStack[commandsStackPtr-1];
    if (c.op == INS){
        deleteCharAtPos(textBuffer, c.pos+1);

    }else if (c.op == DEL){
        insertCharAtPos(textBuffer, c.c, c.pos-1);
    }

    KSDL_updateText(textArea);
    moveCursorAbsolute(c.pos);

    //Pop the command
    commandsStackPtr--;
    executingCommand = 0;
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



///Function called always on a background thread to not block the main one with the rendering
void* runGispOnBuffer(void* data){
    char* path = (char*)data;
    //Compose command
    char cmd[1024];
    sprintf(cmd, "gisp %s", path);

    //Execute it
    FILE* f = popen(cmd, "r");
    consolePanel->text[0] = '\0';

    //Read all the output
    char line[1024*10];
    consolePanel->scrollY = 0;
    while(fgets(line, 1024, f)){
        strcat(consolePanel->text, line);

        int lineHeight = cursor->rect.h;
        int lineCount = KSDL_lineCount(consolePanel);
        int maxLines = (consolePanel->backgroundRect.h/lineHeight) - 1;
        if (lineCount > maxLines){
            consolePanel->scrollY += lineHeight;
        }

        printf("a: %d, b: %d, c: %d\n", lineCount, maxLines, consolePanel->scrollY);

    }
    pclose(f);
    KSDL_updateImage(outputPreview);
    return NULL;
}



void saveAndRunOnBuffer(char* path, char* buffer){
    //Save the file
    writeBufferToFile(path, buffer);

    pthread_t thread;
    pthread_create(&thread, NULL, runGispOnBuffer, (void*)path);
}













/**
 *
 * Vim
 *
 * */

int jumpWord(int dir){
    int bufferLength = strlen(textBuffer);
    int nextWordPos = cursor->pos;
    char currentChar,previousChar;
    do{ nextWordPos += dir;
        if (nextWordPos+1 > bufferLength){break;}
        if (nextWordPos <= 0){break;}
        currentChar = textBuffer[nextWordPos];
        previousChar = textBuffer[nextWordPos-1];
    }while(isspace(currentChar) || (isalnum(currentChar) && isalnum(previousChar)));
    return nextWordPos;
}


void vim_i();
void motionTo(int pos){
    if (vimSubMode == VIM_NONE){
        moveCursorAbsolute(pos);

    }else if (vimSubMode == VIM_CHANGE){
        deleteCharsFromTo(cursor->pos, pos);
        moveCursorAbsolute(cursor->pos);
        vim_i();

    }else if (vimSubMode == VIM_DELETE){
        deleteCharsFromTo(cursor->pos, pos);
        moveCursorAbsolute(cursor->pos);
    }
    KSDL_updateText(textArea);
    vimSubMode = VIM_NONE;
}




void vim_escape(){vimMode = VIM_NORMAL; vimSubMode = VIM_NONE; KSDL_changeCursor(cursor, '_', 0);}

//Submodes
void vim_c(){vimSubMode = VIM_CHANGE;}
void vim_d(){vimSubMode = VIM_DELETE;}
void vim_r(){vimMode = VIM_REPLACE;}

//Movements
void vim_0(){moveCursorAbsolute(KSDL_getLineStart(cursor));}
void vim_$(){moveCursorAbsolute(KSDL_getLineEnd(cursor));}
void vim_w(){motionTo(jumpWord(1));}
void vim_b(){motionTo(jumpWord(-1));}

//Insert mode
void vim_i(){vimMode = VIM_INSERT; KSDL_changeCursor(cursor, '|', -0.5);}
void vim_a(){moveCursor(1, 0, 0); vimMode = VIM_INSERT; KSDL_changeCursor(cursor, '|', -0.5);}
void vim_I(){vim_0(); vim_i();}
void vim_A(){vim_$(); vim_a();}




























/**
 *
 * Main
 *
 * */
int main(int argc, char** argv) {
    //Initialize SDL, Window, Renderer, and Fonts
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO );

    gWindow = SDL_CreateWindow("Gisp Editor 2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_ALLOW_HIGHDPI);
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0);


    int window_w, drawable_w;
    SDL_GetWindowSize(gWindow, &window_w, NULL);
    SDL_GL_GetDrawableSize(gWindow, &drawable_w, NULL);
    dpiScale = drawable_w / window_w;

    //Init TTF
    TTF_Init();
    gFont = TTF_OpenFont("../fonts/FiraCode-Regular.ttf", 14*dpiScale);
    if (gFont == NULL) {fprintf(stderr, "error: font not found\n"); return 1;}

    //Init Image
    int imgFlags = IMG_INIT_PNG;
    if(!(IMG_Init(imgFlags) & imgFlags)){printf("Can't initialize SDL_image lib"); return 1;}

    //Debug text
    char debugBuffer[1024] = "";
    debugText = KSDL_initText(gRenderer, debugBuffer, 0, (WINDOW_H*0.1)*dpiScale, (WINDOW_W*0.8)*dpiScale, (WINDOW_H*0.6)*dpiScale, gFont);

    //Check for input
    char* path;
    if (argc>1){path = argv[1]; readFileToBuffer(path, textBuffer);
    }else{textBuffer[0] = '\0';}

    //Parameters
    int previewPanelWidth = WINDOW_W*0.3;
    int consoleBufferHeight = WINDOW_H*0.3;

    //Console
    char consoleBuffer[1024*10] = "Press CMD+r to run the program.";
    consolePanel = KSDL_initText(gRenderer, consoleBuffer, 0, (WINDOW_H-consoleBufferHeight)*dpiScale, (WINDOW_W-previewPanelWidth)*dpiScale, consoleBufferHeight*dpiScale, gFont);
    consolePanel->backgroundColor.r = 0x00;
    consolePanel->backgroundColor.g = 0x00;
    consolePanel->backgroundColor.b = 0x00;
    consolePanel->backgroundColor.a = 0xff;
    KSDL_setPadding(consolePanel, 16, 16, 0, 0);
    KSDL_setBorder(consolePanel, 2, 20, 20, 20, 255);

    //Output preview
    char outputPath[1024] = "output_raytracer_gisp.png";
    outputPreview = KSDL_initImage(gRenderer, outputPath, (WINDOW_W-previewPanelWidth)*dpiScale, 0, previewPanelWidth*dpiScale, WINDOW_H);

    //Initialize text area
    textArea = KSDL_initText(gRenderer, &textBuffer[0], 0, 0, (WINDOW_W-previewPanelWidth)*dpiScale, (WINDOW_H-consoleBufferHeight)*dpiScale, gFont);
    KSDL_updateText(textArea);
    KSDL_setPadding(textArea, 16, 16, 0, 0);
    KSDL_setBorder(textArea, 2, 20, 20, 20, 255);

    //Initialize cursor
    cursor = KSDL_initCursor(gRenderer, textBuffer, BUFFER_SIZE, gFont);
    moveCursor(0, 0, 0);

    KSDL_changeCursor(cursor, '_', 0);

    //Main Loop
    SDL_StartTextInput();
    SDL_Event event;
    int quit = 0;
    int inputProcessed = 0;
    while (!quit) {
        inputProcessed = 0;
        while (SDL_PollEvent(&event) == 1) {
            switch(event.type){
                //Quit event
                case SDL_QUIT:
                    quit = 1;
                    break;

                //TextInput event
                case SDL_TEXTINPUT:
                    if (inputProcessed == 0) {
                        printf("textinput: %c\n", event.text.text[0]);
                        dLogInt("VIM_MODE", vimMode);
                        dLogInt("VIM_SUBMODE", vimSubMode);
                        if(inputMode == INPUT_SIMPLE || (inputMode == INPUT_VIM && vimMode == VIM_INSERT)){
                            //viminsert/basic input
                            updateTextAreaWithChar(event.text.text[0]);
                        }else if (vimMode == VIM_REPLACE){
                            deleteAfterCursor();
                            updateTextAreaWithChar(event.text.text[0]);
                            moveCursor(-1, 0, 0);
                            vimMode = VIM_NORMAL;
                        }
                    }
                    break;

                //Other special Keydown event not intercepted by textInput
                case SDL_KEYDOWN:
                    if (vimMode == VIM_REPLACE){break;}

                    inputProcessed = 1;
                    printf("Keydown: %s\n", (char *)SDL_GetKeyName(event.key.keysym.sym));
                    dLogInt("VIM_MODE", vimMode);
                    dLogInt("VIM_SUBMODE", vimSubMode);
                    if (event.key.keysym.mod & KMOD_GUI){
                        switch(event.key.keysym.sym){
                            case SDLK_s: writeBufferToFile(path, textBuffer); break;
                            case SDLK_r: saveAndRunOnBuffer(path, textBuffer); break;
                            case SDLK_z: undo(); break;
                        }
                    }

                    if (inputMode == INPUT_SIMPLE || (inputMode == INPUT_VIM && vimMode == VIM_INSERT)){
                        //Normal basic input are handled in SDL_TEXTINPUT above,
                        //Here handles normal movements and special keys
                        if (event.key.keysym.mod & KMOD_SHIFT){
                            //Command modifiers
                            switch(event.key.keysym.sym){
                                case SDLK_LEFT:    moveCursor(-1,  0, 1); break;
                                case SDLK_RIGHT:   moveCursor( 1,  0, 1); break;
                                case SDLK_UP:      moveCursor( 0, -1, 1); break;
                                case SDLK_DOWN:    moveCursor( 0,  1, 1); break;
                                default: inputProcessed = 0;
                            }
                        }else{
                            //No modifiers
                            switch(event.key.keysym.sym){
                                case SDLK_ESCAPE:    vim_escape(); break;
                                case SDLK_BACKSPACE: deleteBeforeCursor(); break;
                                case SDLK_DELETE:    deleteAfterCursor(); break;
                                case SDLK_RETURN:    updateTextAreaWithChar('\n'); break;
                                case SDLK_LEFT:      moveCursor(-1,  0, 0); break;
                                case SDLK_RIGHT:     moveCursor( 1,  0, 0); break;
                                case SDLK_UP:        moveCursor( 0, -1, 0); break;
                                case SDLK_DOWN:      moveCursor( 0,  1, 0); break;
                                default: dLog("Key not recognized in simple/viminput mode:");dLog((char *)SDL_GetKeyName(event.key.keysym.sym));inputProcessed = 0;
                            }
                        }

                    }else if (inputMode == INPUT_VIM && vimMode == VIM_NORMAL){
                        //Vim bindings
                        if (event.key.keysym.mod & KMOD_SHIFT){
                            switch(event.key.keysym.sym){
                                //Inserts
                                case SDLK_i:  vim_I(); break;
                                case SDLK_a:  vim_A(); break;

                                //Movements
                                case SDLK_4:  vim_$(); break;
                            }
                        }else{
                            switch(event.key.keysym.sym){
                                case SDLK_ESCAPE: vim_escape(); break;
                                case SDLK_r:      vim_r(); break;

                                //Inserts
                                case SDLK_i:  vim_i(); break;
                                case SDLK_a:  vim_a(); break;

                                //Movements
                                case SDLK_w:  vim_w(); break;
                                case SDLK_b:  vim_b(); break;
                                case SDLK_0:  vim_0(); break;
                                case SDLK_h:  moveCursor(-1, 0, 0); break;
                                case SDLK_j:  moveCursor( 0, 1, 0); break;
                                case SDLK_k:  moveCursor( 0,-1, 0); break;
                                case SDLK_l:  moveCursor( 1, 0, 0); break;

                                //Submodes switch
                                case SDLK_c:  vim_c(); break;
                                case SDLK_d:  vim_d(); break;


                                case SDLK_u: undo(); break;


                                default: dLog("Key not recognized in vim mode (special key):");dLog((char *)SDL_GetKeyName(event.key.keysym.sym));inputProcessed = 0;
                            }
                        }
                    }

                    updateDebugText();
            }
        }



        //Clear the screen
        SDL_RenderClear(gRenderer);

        //Render text area
        KSDL_drawText(textArea);
        KSDL_drawCursor(cursor, textArea);

        //Render console
        KSDL_updateText(consolePanel);
        KSDL_drawText(consolePanel);

        //Render preview
        KSDL_drawImage(outputPreview);

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
