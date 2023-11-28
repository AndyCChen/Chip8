#include <stdio.h>
#include <stdbool.h>
#include "SDL.h"

#include "../includes/display.h"
#include "../includes/chip8.h"

static SDL_Window *gWindow = NULL;
static SDL_Renderer *gRenderer = NULL;

static int SCREEN_WIDTH = PIXELS_W;
static int SCREEN_HEIGHT = PIXELS_H;

// array of rectangles representing a pixel for the display
static SDL_Rect Display [PIXELS_W * PIXELS_H];

// keeps track of the on or off state of a rectangle (pixel)
// 0: off, pixel is black
// 1: on, pixel is white
static uint8_t Display_buffer [PIXELS_W * PIXELS_H];

// flag that is used by draw and clear instructions to signal when the display needs to be updated
static bool update_flag = false;

int display_init(int display_scale_factor)
{
   // scale up resolution based on scale factor
   SCREEN_WIDTH *= display_scale_factor;
   SCREEN_HEIGHT  *= display_scale_factor;

   // initialize SDL
   if (SDL_Init(SDL_INIT_VIDEO) < 0)
   {
      printf("SDL could not initialize! SDL Error %s\n", SDL_GetError());
      return -1;
   }
   
   gWindow = SDL_CreateWindow("Chip 8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,  SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
   if (gWindow == NULL)
   {
      printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError());
      return -1;
   }

   gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
   if (gRenderer == NULL)
   {
      printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
      return -1;
   }

   // initialze display with 64 * 32 rectangles (pixels)
   // rectangles here represent a single pixel
   int number_of_rects = PIXELS_W * PIXELS_H; // number of rectangles to render
   int rect_w = SCREEN_WIDTH / PIXELS_W;      // width of a rect
   int rect_h = SCREEN_HEIGHT / PIXELS_H;     // height of a rect
   int rect_pos_x = 0;                        // x position of a rect
   int rect_pos_y = 0;                        // y position of a rect 
   int row_count = 0;                         // row position of a rect
   for (int rect_number = 0; rect_number < number_of_rects; ++rect_number)
   {
      rect_pos_x = ( rect_w * (rect_number % PIXELS_W) ); 
      if (rect_number % PIXELS_W == 0)
      {
         rect_pos_y = ( rect_h * (row_count++) ); // post decrement make sure to use initial row_count value of 0 on first pass
      }

      Display[rect_number] = (SDL_Rect)
      { 
         .x = rect_pos_x,
         .y = rect_pos_y, 
         .w = rect_w, 
         .h = rect_h 
      };
   }

   return 0;
}

void display_close()
{
   SDL_DestroyRenderer(gRenderer);
   SDL_DestroyWindow(gWindow);
   gRenderer = NULL;
   gWindow = NULL;

   SDL_Quit();
   printf("closing sdl!\n");
}

void display_update()
{
   SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
   SDL_RenderClear(gRenderer);

   SDL_SetRenderDrawColor(gRenderer, 0xFF, 0x00, 0x00, 0xFF);
   SDL_RenderDrawRects(gRenderer, &Display, PIXELS_W * PIXELS_H);  // pixels_w  * pixels_h is the number of rectangles being drawn

   SDL_RenderPresent(gRenderer);

   // reset update flag back to false when render updates are done
   update_flag = false;
}

void display_draw(uint8_t x_pos, uint8_t y_pos, uint8_t sprite_height)
{
   // first clear the VF flag incase it was previously set to 1
   myChip8.V[0xF] = 0;

   const int sprite_width = 8; // sprites are always 8 bits (pixels) wide

   uint8_t *sprite = &myChip8.ram[myChip8.I]; // sprite data at starting address I in ram

   uint8_t *sprite_origin = &Display_buffer [( y_pos * PIXELS_W  ) + x_pos]; // initial position origin of sprite in the display

   // iterate through entire sprite byte by byte
   for(int sprite_byte = 0; sprite_byte < sprite_height; ++sprite_byte)
   {
      // stop drawing if we go over the max pixel height
      if (y_pos + sprite_byte >= PIXELS_H)
      {
         break;
      }

      // iterate through sprit_byte bit by bit
      for (int sprite_bit = 0; sprite_bit < sprite_width; ++sprite_bit)
      {
         // stop drawing if we go over the max pixel width
         if (x_pos + sprite_bit >= PIXELS_W)
         {
            break;
         }

         // state of the display pixel that we wish to XOR with
         uint8_t display_pixel_state = sprite_origin[( sprite_byte * sprite_width ) + sprite_bit];

         // state of sprite pixel that we wish to XOR with
         uint8_t sprite_pixel_state = sprite[sprite_byte] & ( 1 << ( 7 - sprite_bit ) );

         // set VF flag if both states are on (eqauls 1)
         if (display_pixel_state == 1 && sprite_pixel_state == 1)
         {
            myChip8.V[0xF] = 1;
         }

         // xor display buffer pixel state with sprite pixel state
         sprite_origin[(sprite_byte * sprite_width) + sprite_bit] = display_pixel_state ^ sprite_pixel_state;
      }
   }

   // signal that we want to update the display
   update_flag = true;
}

void display_clear()
{
   // set all display pixels to off state
   memset(Display_buffer, 0, PIXELS_H * PIXELS_W);

   // signal that we want to update display
   update_flag = true;
}

bool display_get_update_flag()
{
   return update_flag;
}