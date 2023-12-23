#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>

#include "SDL.h"

// contains all utilities for I/O of display for chip8

// chip8 display resolution is 64 by 32 pixels

#define PIXELS_W 64
#define PIXELS_H 32

// audio settings for square wave beep generator

#define VOLUME 3000
#define SAMPLES_PER_SEC 44100
#define FREQUENCY 256 // freq in hz
#define PERIOD ( SAMPLES_PER_SEC / FREQUENCY ) // number of samples in a period
#define HALF_PERIOD  ( PERIOD / 2 ) // number of samples that a wave is held low or high

// initialize application window
bool display_init(int display_scale_factor);

void display_set_chip8_viewport(int x_pos, int y_pos);

// free memory
void display_close(void);

// update pixel states with the display buffer
void display_update(void);

// draw to the display buffer
// takes in the initial x and y position coordinates of sprite placement
// and the height of the sprite ranging from 1-15 pixels
void display_draw(uint8_t x_pos, uint8_t y_pos, uint8_t sprite_height);

// call SDL_RenderClear directly to clear the display
// does not affect the display buffer
void display_clear(void);

// clear display buffer so that a subsequent call to display update will clear the screen
// call display_clear instead to clear the screen directly
void display_clear_buffer(void);

// pause_on: non-zero to pause audio, 0 to unpause
void display_pause_audio_device(int pause_on);

SDL_Window *display_get_window(void);

SDL_Renderer *display_get_renderer(void);

// present renderer to the display by calling SDL_RenderPresent
void display_present(void);

#endif