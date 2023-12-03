//#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "SDL.h"
#include "./includes/chip8.h"
#include "./includes/display.h"

// process key inputs when key is pressed
void process_key_input_down(SDL_Event *e); 

// process key inputs when key is released
void process_key_input_up(SDL_Event *e); 

int main( int argc, char *argv[])
{
	srand(time(NULL));

	// default clock speed is 500 hz
	uint32_t chip8_clock_rate = 500;

	// default scaling factor of the 64 by 32 pixel display
	// 15 is the default
	uint32_t display_scale =  15;

	// begin handling command line arguments with getopt
	extern char *optarg;
	int option;
	int clock_rate_flag = 0, display_scale_flag = 0, rom_path_flag = 0;
	bool log_flag = false;
	const char *rom_path_arg = NULL, *clock_rate_arg = NULL, *display_scale_arg = NULL;

	while ( ( option = getopt(argc, argv, "c:d:p:l") ) != -1 )
	{
		switch ( option )
		{
			case 'c': 
			{
				clock_rate_flag = 1;
				clock_rate_arg = optarg;
				break;
			}
			case 'd': 
			{
				display_scale_flag = 1;
				display_scale_arg = optarg;
				break;
			}
			case 'p':
			{
				rom_path_flag = 1;
				rom_path_arg = optarg;
				break;
			}
			case 'l': log_flag = true; break;
			default:
			{
				printf("Usage: %s [-p rom_path] [-c clock_rate] [-d display_scale]\n", argv[0]);
				printf("-p sets the path to the rom to run, is a required argument\n");
				printf("-c set the clock rate, is optional and defaults to %d hz\n", chip8_clock_rate);
				printf("-d sets the display scale size, is optional defaults to %d\n", display_scale);
				printf("-l enables the disassembler logs to the console\n");
				return EXIT_FAILURE;
			}
		}
	}

	if (rom_path_flag == 0)
	{
		printf("Missing [-p rom_path] arugment!\n");
		return EXIT_FAILURE;
	}

	if (clock_rate_flag == 1) chip8_clock_rate = atoi(clock_rate_arg);

	if (display_scale_flag == 1) display_scale = atoi(display_scale_arg);

	// end command line handling

	// initialize chip8
	chip8_reset();

	// attempt to load rom file into chip8 ram
	if (!chip8_load_rom(rom_path_arg)) return EXIT_FAILURE;

	printf("program loaded!\n");

	// initialize display scaled to the display scale factor,default value of 15
	if (display_init(display_scale) < 0) {
		return EXIT_FAILURE;
	}

	SDL_Event e;
   bool quit = false; 

	// timers to lock chip8 speed into a specified clock speed

	float delta_time = 0; // seconds passed in beween loop iterations
	clock_t current_time; 
	clock_t previous_time = clock();

	// main loop
   while(!quit)
   { 
      while(SDL_PollEvent( &e ))
      { 
         //printf("polling\n");
         if (e.type == SDL_QUIT)
         {  
				quit = true;
			}
			else if (e.type == SDL_KEYDOWN)
			{	
				process_key_input_down(&e);
			}
			else if (e.type == SDL_KEYUP)
			{
				process_key_input_up(&e);
			}
      }
      
		current_time = clock();
		delta_time +=  (float) ( current_time - previous_time ) / CLOCKS_PER_SEC;
		previous_time = current_time;

		// this loop runs at specified hertz rate
		while ( delta_time >= (float) 1 / chip8_clock_rate )
		{
			chip8_run_cycle(log_flag);
			
			// update display if update flag is set to true by the draw or clear instructions
			if ( display_get_update_flag() )
			{
				display_update();
			}
				
			delta_time -= (float) 1 / chip8_clock_rate;
		}
   }

	display_close();
	
	return EXIT_SUCCESS;
}

void process_key_input_down(SDL_Event *e)
{
	switch ( e->key.keysym.scancode )
	{
		case SDL_SCANCODE_1: chip8_set_key_down(0x1); break;
		case SDL_SCANCODE_2: chip8_set_key_down(0x2); break;
		case SDL_SCANCODE_3: chip8_set_key_down(0x3); break;
		case SDL_SCANCODE_4: chip8_set_key_down(0xC); break;
		case SDL_SCANCODE_Q: chip8_set_key_down(0x4); break;
		case SDL_SCANCODE_W: chip8_set_key_down(0x5); break;
		case SDL_SCANCODE_E: chip8_set_key_down(0x6); break;
		case SDL_SCANCODE_R: chip8_set_key_down(0xD); break;
		case SDL_SCANCODE_A: chip8_set_key_down(0x7); break;
		case SDL_SCANCODE_S: chip8_set_key_down(0x8); break;
		case SDL_SCANCODE_D: chip8_set_key_down(0x9); break;
		case SDL_SCANCODE_F: chip8_set_key_down(0xE); break;
		case SDL_SCANCODE_Z: chip8_set_key_down(0xA); break;
		case SDL_SCANCODE_X: chip8_set_key_down(0x0); break;
		case SDL_SCANCODE_C: chip8_set_key_down(0xB); break;
		case SDL_SCANCODE_V: chip8_set_key_down(0xF); break;
		default: break;
	}
}

void process_key_input_up(SDL_Event *e)
{
	switch ( e->key.keysym.scancode )
	{
		case SDL_SCANCODE_1: chip8_set_key_up(0x1); break;
		case SDL_SCANCODE_2: chip8_set_key_up(0x2); break;
		case SDL_SCANCODE_3: chip8_set_key_up(0x3); break;
		case SDL_SCANCODE_4: chip8_set_key_up(0xC); break;
		case SDL_SCANCODE_Q: chip8_set_key_up(0x4); break;
		case SDL_SCANCODE_W: chip8_set_key_up(0x5); break;
		case SDL_SCANCODE_E: chip8_set_key_up(0x6); break;
		case SDL_SCANCODE_R: chip8_set_key_up(0xD); break;
		case SDL_SCANCODE_A: chip8_set_key_up(0x7); break;
		case SDL_SCANCODE_S: chip8_set_key_up(0x8); break;
		case SDL_SCANCODE_D: chip8_set_key_up(0x9); break;
		case SDL_SCANCODE_F: chip8_set_key_up(0xE); break;
		case SDL_SCANCODE_Z: chip8_set_key_up(0xA); break;
		case SDL_SCANCODE_X: chip8_set_key_up(0x0); break;
		case SDL_SCANCODE_C: chip8_set_key_up(0xB); break;
		case SDL_SCANCODE_V: chip8_set_key_up(0xF); break;
		default: break;
	}
}