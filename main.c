//#include <stdint.h>
#include <stdio.h>

#include "SDL.h"
#include "./includes/chip8.h"

int main( int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("Missing rom path argument!\n");
		return EXIT_FAILURE;
	}
	
	chip8_reset();

	if (!chip8_load_rom(argv[1]))
	{
		return EXIT_FAILURE;
	}

	printf("program loaded!\n");

	return EXIT_SUCCESS;
}
