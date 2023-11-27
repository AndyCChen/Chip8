//#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
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

	for (int n = 0; n < 16; ++n)
		chip8_run_cycle();

	if (display_init(18) < 0) {
		return EXIT_FAILURE;
	}

	SDL_Event e;
   int quit = 0; 
   while(quit == 0)
   { 
      while(SDL_PollEvent( &e ))
      { 
         //printf("polling\n");
         if(e.type == SDL_QUIT) 
            quit = 1;
      } 

		display_update();
      //printf("execute cpu cycle here.\n");
   }

	display_close();
	
	return EXIT_SUCCESS;
}
