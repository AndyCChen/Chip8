#ifndef DISPLAY_H
#define DISPLAY_H

// contains all sdl2 utilities for handling the display for chip8

// chip8 display resolution is 64 by 32 pixels
#define PIXELS_W 64
#define PIXELS_H 32

// initialize the display
// return 0 on success else return negative value
// takes in a scale factor to scale the 64 by 32 pixel display resolution
int display_init(int);

// free memory
void display_close();

void display_update();

// draw to the display
// takes in the initial x and y position coordinates of sprite placement
// and the height of the sprite ranging from 1-15 pixels
void display_draw(uint8_t x_pos, uint8_t y_pos, uint8_t sprite_height);

// set all display pixels to off
void display_clear();

#endif