#include <stdio.h>
#include "SDL.h"

#include "../includes/display.h"

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;

int SCREEN_WIDTH = PIXELS_W;
int SCREEN_HEIGHT = PIXELS_H;

SDL_Rect Display [PIXELS_W * PIXELS_H];

int display_init(int display_scale_factor)
{
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
   SDL_RenderDrawRects(gRenderer, Display, PIXELS_W * PIXELS_H);  // pixels_w  * pixels_h is the number of rectangles being drawn

   SDL_RenderPresent(gRenderer);
}