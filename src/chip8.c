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
   myChip8.PC = PROGRAM_START;
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
   // 16 bit opcode
   uint16_t opcode = myChip8.ram[myChip8.PC];     // grab first 8 bits of opcode
   opcode = opcode << 8;                          
   opcode = opcode | myChip8.ram[myChip8.PC + 1]; // bitwise or the first 8 bits with second 8 bits to form 16 bit opcode

   // first 4 bits (nibble) of a opcode
   uint8_t first_nibble = myChip8.ram[myChip8.PC];
   first_nibble = first_nibble >> 4; // left shift 4 bits to get rid of second nibble
   printf("%04x %04x ", myChip8.PC, opcode);
   
   switch(first_nibble)
   {
      case 0x00: 
      {
         if (opcode == 0x00E0)
            printf("00E0 opcode\n");
         else if (opcode == 0x00EE)
            printf("00EE opcode\n");
         break;
      } 
      case 0x01: 
      {
         printf("1NNN opcode\n"); 
         break;
      }
      case 0x02: printf("2 opcode\n"); break;
      case 0x03: printf("3 opcode\n"); break;
      case 0x04: printf("4 opcode\n"); break;
      case 0x05: printf("5 opcode\n"); break;
      case 0x06: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t NN = opcode & 0x00FF;

         printf("%01x %01x %02x opcode\n", first_nibble, X, NN); 
         break;
      }
      case 0x07: printf("7 opcode\n"); break;
      case 0x08: printf("8 opcode\n"); break;
      case 0x09: printf("9 opcode\n"); break;
      case 0x0A: 
      {
         uint16_t NNN = opcode & 0x0FFF;
         myChip8.I = NNN;
         
         printf("%01x %03x opcode\n", first_nibble, NNN);
         break;
      }
      case 0x0B: printf("B opcode\n"); break;
      case 0x0C: printf("C opcode\n"); break;
      case 0x0D: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t Y = (opcode & 0x00F0) >> 4;
         uint8_t N = (opcode & 0x000F);

         printf("%01x %01x %01x %01x\n", first_nibble, X, Y, N); 
         break;
      }
      case 0x0E: printf("E opcode\n"); break;
      case 0x0F: printf("F opcode\n"); break;
   }
   
   // increment program counter to point to next intruction (next 2 bytes)
   myChip8.PC += 2;
}
