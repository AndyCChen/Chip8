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

// initialize the display
// return 0 on success else return negative value
// takes in a scale factor to scale the 64 by 32 pixel display resolution
int display_init(int);

// free memory
void display_close();

// update display with buffer
void display_update();

// draw to the display buffer
// takes in the initial x and y position coordinates of sprite placement
// and the height of the sprite ranging from 1-15 pixels
void display_draw(uint8_t x_pos, uint8_t y_pos, uint8_t sprite_height);

// set all display pixels to off
void display_clear();

// pause_on: non-zero to pause audio, 0 to unpause
void display_pause_audio_device(int pause_on);

SDL_Window *display_get_window();

SDL_Renderer *display_get_renderer();

#endif