#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../includes/chip8.h"

Chip8 myChip8;

void chip8_reset()
{
   memset(myChip8.ram, 0, RAM_SIZE);
   memset(myChip8.V, 0, V_REGISTERS);
   memset(myChip8.stack, 0, MAX_STACK_LEVEL);
   myChip8.I = 0;
   myChip8.PC = 0;
   myChip8.sp = 0;
   myChip8.delay_timer = 0;
   myChip8.sound_timer = 0;
}

int chip8_load_rom(const char const *file_path) 
{
	FILE *file = fopen(file_path, "rb");

	if (!file) {
		printf("Cannot open file %s\n", file_path);
      return 0;
	} 

   fseek(file, 0, SEEK_END);
   int file_size = ftell(file);
   rewind(file);

   if (file_size == 0) {
      printf("Reading empty file\n");
      return 0;
   }

   if (file_size > RAM_SIZE - PROGRAM_START)
   {
      printf("Rom to large to read into chip8 ram!\n");
      return 0;
   }

   int bytes_read = fread(myChip8.ram + PROGRAM_START, 1, file_size, file);

   if (bytes_read != file_size) {
      printf("File reading error!\n");
      return 0;
   }

	fclose(file);
   return bytes_read;
}

void chip8_run_cycle()
{
   for (int i = 0; i < 5; ++i)
		printf("%08x %08x\n", myChip8.ram[PROGRAM_START+i], myChip8.ram[PROGRAM_START+i] << 2);
}
