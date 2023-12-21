//#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "SDL.h"

#include "./includes/chip8.h"
#include "./includes/display.h"
#include "./includes/gui.h"

void process_key_input_down(SDL_Event *e); 
void process_key_input_up(SDL_Event *e); 
bool process_command_line_args(int argc, char *argv[]);

static bool pause_flag = false, cycle_step_flag = false, log_flag = false;

static const char *rom_path_arg = NULL;

// default clock speed is 500 hz
static uint32_t chip8_clock_rate = 500;

// default scaling factor of the 64 by 32 pixel display
// 15 is the default
static uint32_t display_scale =  15;

int main( int argc, char *argv[])
{
	srand(time(NULL));

	if( !process_command_line_args(argc, argv) ) return EXIT_FAILURE;

	// initialize chip8
	chip8_reset();

	// attempt to load rom file into chip8 ram
	if (!chip8_load_rom(rom_path_arg)) return EXIT_FAILURE;

	printf("program loaded!\n");

	// initialize display scaled to the display scale factor,default value of 15
	if (display_init(display_scale) < 0) return EXIT_FAILURE;

	gui_init();

	SDL_Event event;
   bool quit_flag = false; 

	// timers to lock chip8 speed into a specified clock speed

	float delta_time = 0; // seconds passed in beween loop iterations
	clock_t current_time; 
	clock_t previous_time = clock();

	// main loop
   while(!quit_flag)
   { 
		restart:
		
		// process sdl events in the window
		gui_input_begin();
		while(SDL_PollEvent( &event ))
      { 
         if (event.type == SDL_QUIT)
         {  
				quit_flag = true;
				break;
			}
			else if (event.type == SDL_KEYDOWN)
			{	
				process_key_input_down(&event);
			}
			else if (event.type == SDL_KEYUP)
			{
				process_key_input_up(&event);
			}

			gui_handle_event(&event);
      }
		gui_input_end();

		// exit if close window is pressed
		if (quit_flag) break;

		// this block only runs if chip8 is paused by pressing f5
		if (pause_flag)
		{
			// step through a single cycle when space is pressed
			if (cycle_step_flag)
			{
				chip8_run_cycle(log_flag);
				cycle_step_flag = false;
			}

			gui_create_widgets(); // declare and initialize gui elements
			display_clear();      // clear the display before draw
			display_update();     // set render rectangles (pixels) with display buffer 
			gui_draw();           // draw the gui elements
			display_present();    // finally present the render to display
			goto restart;         // jump back up to restart so we can continually poll events while paused
		}

		// main chip8 loop process
		current_time = clock();
		delta_time +=  (float) ( current_time - previous_time ) / CLOCKS_PER_SEC;
		if ( delta_time >= 0.1 ) delta_time = 0.1; // limit delta time to 100ms
		previous_time = current_time;

		// this loop runs at specified hertz rate
		while ( delta_time >= (float) 1 / chip8_clock_rate )
		{
			chip8_run_cycle(log_flag);
				
			delta_time -= (float) 1 / chip8_clock_rate;
		}

		gui_create_widgets(); // declare and initialize gui elements
		display_clear();      // clear the display before draw
		display_update();     // set render rectangles (pixels) to correct the color with display buffer 
		gui_draw();           // draw the gui elements
		display_present();    // finally present the render to display
   }

	gui_close();
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
		case SDL_SCANCODE_F5: 
		{
			pause_flag = !pause_flag;
			if (pause_flag) printf("Paused, press space to step through a single instruction or press f5 again to resume.\n");
			else printf("Resuming chip8\n"); 
			break;
		}
		case SDL_SCANCODE_SPACE: cycle_step_flag = true; break;
		default: break;
	}
}

bool process_command_line_args(int argc, char *argv[])
{
	extern char *optarg;
	int option;
	int clock_rate_flag = 0, display_scale_flag = 0, rom_path_flag = 0;
	const char *clock_rate_arg = NULL, *display_scale_arg = NULL;

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
				printf("Usage: chip8.exe [-p rom_path] [-c clock_rate] [-d display_scale]\n");
				printf("\t -p sets the path to the rom to run, is a required argument\n");
				printf("\t -c optional, set the clock rate, defaults to %d hz\n", chip8_clock_rate);
				printf("\t -d optional, sets the display scale size, defaults to %d\n", display_scale);
				printf("\t -l optional, enables the disassembler logs to the console\n");
				return false;
			}
		}
	}

	if (rom_path_flag == 0)
	{
		printf("Missing [-p rom_path] arugment!\n");
		return false;
	}

	if (clock_rate_flag == 1) chip8_clock_rate = atoi(clock_rate_arg);

	if (display_scale_flag == 1) display_scale = atoi(display_scale_arg);

	return true;
}