//#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SDL.h"
#include "./includes/chip8.h"
#include "./includes/display.h"

int main( int argc, char *argv[])
{
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

/* 	for (int n = 0; n < 16; ++n)
		chip8_run_cycle(); */

	if (display_init(18) < 0) {
		return EXIT_FAILURE;
	}

	SDL_Event e;
   int quit = 0; 

	float delta_time = 0; // seconds passed in beween loop iterations

	// number of clock ticks elapsed since epoch
	clock_t current_time = time();
	clock_t previous_time = current_time;
   while(quit == 0)
   { 
      while(SDL_PollEvent( &e ))
      { 
         //printf("polling\n");
         if(e.type == SDL_QUIT) 
            quit = 1;
      }

		delta_time += ( current_time - previous_time ) / CLOCKS_PER_SEC;
		if ( delta_time >= (float) 1 / 60 )
		{
			chip8_run_cycle();

			// update display if update flag is set to true by the draw or clear instructions
			if ( display_get_is_update() ) 
				display_update();

			delta_time -= (float) 1 / 60;
		}
		previous_time = current_time;
		current_time = time();
   }

	display_close();
	
	return EXIT_SUCCESS;
}
