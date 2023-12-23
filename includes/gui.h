#ifndef GUI_H
#define GUI_H

#include "SDL.h"

#define GUI_STACK_WIDGET_W 110
#define GUI_MEMORY_WIDGET_W 225
#define GUI_CPU_STATE_WIDGET_W 150

// initialize gui context for nuklear 
void gui_init();

// free gui memory
void gui_close();

void gui_input_begin();

void gui_input_end();

void gui_handle_event(SDL_Event *event);

// declare and initialize all gui window and widgets
void gui_create_widgets();

// draw gui by calling nk_sdl_render
void gui_draw();

#endif