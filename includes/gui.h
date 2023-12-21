#ifndef GUI_H
#define GUI_H

#include "SDL.h"

// initialize gui context for nuklear 
void gui_init();

// free gui memory
void gui_close();

void gui_input_begin();

void gui_input_end();

void gui_handle_event(SDL_Event *event);

// create gui and draw to display
// this function calls display_update to draw to the screen
void gui_create_and_update();

#endif