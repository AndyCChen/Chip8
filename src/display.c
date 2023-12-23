#include <stdio.h>
#include <stdbool.h>
#include "SDL.h"

#include "../includes/display.h"
#include "../includes/chip8.h"
#include "../includes/gui.h"

static SDL_Window *gWindow = NULL;
static SDL_Renderer *gRenderer = NULL;

static SDL_AudioSpec want, have;
static SDL_AudioDeviceID device_id;

// array of rectangles representing a pixel for the display
static SDL_Rect Display [PIXELS_W * PIXELS_H];

// keeps track of the on or off state of a rectangle (pixel)
// 0: off, pixel is black
// 1: on, pixel is white
static uint8_t Display_buffer [PIXELS_W * PIXELS_H];

// callback for sdl to use for generating sound
static void audio_callback(void* userdata, uint8_t* stream, int streamSize)
{
   Sint16 *audioBuffer = (Sint16*) stream;
   int audioBufferLength = streamSize / 2; // streamSize in bytes divide by 2 for length of 16 bit int array
   size_t *runningSampleIndex = (size_t*) userdata; // track the current sample across writes to the buffer
   int sampleValue;

   for (int sampleIndex = 0; sampleIndex < audioBufferLength; ++sampleIndex)
   {
      sampleValue = ( ( *runningSampleIndex / HALF_PERIOD ) % 2 ) ? VOLUME : -VOLUME;
      audioBuffer[sampleIndex] = sampleValue;
      (*runningSampleIndex)++;
   }
}

bool display_init(int display_scale_factor)
{
   // scale up resolution based on scale factor

   const int VIEWPORT_W = PIXELS_W * display_scale_factor;
   const int VIEWPORT_H = PIXELS_H * display_scale_factor;

   // initialize SDL
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
   {
      printf("SDL could not initialize! SDL Error %s\n", SDL_GetError());
      return false;
   }
   
   const int LEFT_OFFSET = GUI_STACK_WIDGET_W + GUI_MEMORY_WIDGET_W + GUI_CPU_STATE_WIDGET_W + 1; // width ocupied by gui located left of the chip8 viewport

   gWindow = SDL_CreateWindow("Chip 8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LEFT_OFFSET + VIEWPORT_W,  VIEWPORT_H, SDL_WINDOW_SHOWN);
   if (gWindow == NULL)
   {
      printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError());
      return false;
   }

   gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
   if (gRenderer == NULL)
   {
      printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
      return false;
   }

   // initialze display with 64 * 32 rectangles (pixels)
   // rectangles here represent a single pixel
   int number_of_rects = PIXELS_W * PIXELS_H;
   int rect_w = VIEWPORT_W / PIXELS_W;
   int rect_h = VIEWPORT_H / PIXELS_H;
   int rect_pos_x = 0;
   int rect_pos_y = 0;
   int row_count = 0; // row position of a rect
   for (int rect_number = 0; rect_number < number_of_rects; ++rect_number)
   {
      rect_pos_x = ( rect_w * (rect_number % PIXELS_W) ) + LEFT_OFFSET; 
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

   // initialize sdl audio

   static size_t runningSampleIndex = 0; // track the current sample across writes to the buffer

   SDL_memset( &want, 0, sizeof( want ) );
   want.freq = SAMPLES_PER_SEC;
   want.format = AUDIO_S16SYS;
   want.channels = 1;
   want.samples = 2048;
   want.callback = audio_callback;
   want.userdata = &runningSampleIndex;
   
   // attempt to open audio device, error if it returns 0
   if ( ( device_id = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0) ) == 0 )
   {
      printf("SDL could not get an audio device! SDL Error: %s\n", SDL_GetError());
      return false;
   }
   
   if (have.format != AUDIO_S16SYS)
   {
      printf("SDL could not get desired audio format! SDL Error: %s\n", SDL_GetError());
      return false;
   }

   return true;
}

void display_close()
{
   SDL_DestroyRenderer(gRenderer);
   SDL_DestroyWindow(gWindow);
   gRenderer = NULL;
   gWindow = NULL;

   SDL_CloseAudioDevice(device_id);

   SDL_Quit();
   printf("closing sdl!\n");
}

void display_update()
{
   for (int pixel = 0; pixel < PIXELS_H * PIXELS_W; ++pixel)
   {
      // if pixel on set color to white
      if (Display_buffer[pixel])
      {
         SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
         SDL_RenderFillRect(gRenderer, &Display[pixel]);
      }
      // else pixel is off so we set color to black
      else
      {
         SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
         SDL_RenderFillRect(gRenderer, &Display[pixel]);
      }
   }
}

void display_present()
{
   SDL_RenderPresent(gRenderer);
}

void display_draw(uint8_t x_pos, uint8_t y_pos, uint8_t sprite_height)
{
   // first clear the VF flag incase it was previously set to 1
   myChip8.V[0xF] = 0;

   const int sprite_width = 8; // sprites are always 8 bits (pixels) wide

   uint8_t *sprite = &myChip8.ram[myChip8.I]; // sprite data at starting address I in ram

   uint16_t display_sprite_origin = ( y_pos * PIXELS_W  ) + x_pos; // position in display buffer to draw sprite to

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
         uint8_t display_pixel_state = Display_buffer[display_sprite_origin + (sprite_byte * PIXELS_W) + sprite_bit];
         
         // state of sprite pixel that we wish to XOR with
         uint8_t sprite_pixel_state = sprite[sprite_byte] & ( 1 << ( 7 - sprite_bit ) ); // extract the specific bit with a bitmask
         sprite_pixel_state = sprite_pixel_state >> ( 7 - sprite_bit );
   
         // set VF flag if both states are on (equals 1)
         if (display_pixel_state == 1 && sprite_pixel_state == 1)
         {
            myChip8.V[0xF] = 1;
         }

         // xor display buffer pixel state with sprite pixel state
         Display_buffer[display_sprite_origin + (sprite_byte * PIXELS_W) + sprite_bit] = display_pixel_state ^ sprite_pixel_state;
      }
   }
}

void display_clear()
{
   SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
   SDL_RenderClear(gRenderer);
}

void display_clear_buffer()
{
   // set all display pixels to off state
   memset(Display_buffer, 0, PIXELS_H * PIXELS_W);
}

void display_pause_audio_device(int pause_on)
{
   SDL_PauseAudioDevice(device_id, pause_on);
}

SDL_Window *display_get_window()
{
   return gWindow;
}

SDL_Renderer *display_get_renderer()
{
   return gRenderer;
}