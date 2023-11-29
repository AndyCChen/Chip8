//#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#include "SDL.h"
#include "./includes/chip8.h"
#include "./includes/display.h"

int main( int argc, char *argv[])
{
	// default clock speed is 500 hz
	uint32_t chip8_clock_speed = 500;

	// default scaling factor of the 64 by 32 pixel display
	// 15 is the default
	uint32_t display_scale =  15;

	// path to a .ch8 rom, is a required argument
	const char *path = "";

	if (argc == 1)
	{
		printf("Missing rom path argument!\n");
		return EXIT_FAILURE;
	}
	
	// initialize chip8
	chip8_reset();

	if (!chip8_load_rom(argv[1]))
	{
		return EXIT_FAILURE;
	}

	printf("program loaded!\n");

	if (display_init(18) < 0) {
		return EXIT_FAILURE;
	}

	SDL_Event e;
   int quit = 0; 

	float delta_time = 0; // seconds passed in beween loop iterations

	// number of clock ticks elapsed since epoch
	clock_t current_time;
	clock_t previous_time = clock();

	// main loop
   while(quit == 0)
   { 
      while(SDL_PollEvent( &e ))
      { 
         //printf("polling\n");
         if(e.type == SDL_QUIT)
            quit = 1;
      }
      
		current_time = clock();
		delta_time +=  (float) ( current_time - previous_time ) / CLOCKS_PER_SEC;
		previous_time = current_time;

		// this loop runs at specified hertz rate
		while ( delta_time >= (float) 1 / 60 )
		{
			chip8_run_cycle();
			
			// update display if update flag is set to true by the draw or clear instructions
			if ( display_get_update_flag() )
			{
				display_update();
			}
				
			delta_time -= (float) 1 / 60;
		}
   }

	display_close();
	
	return EXIT_SUCCESS;
}
