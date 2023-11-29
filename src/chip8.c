#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../includes/chip8.h"
#include "../includes/display.h"

Chip8 myChip8;

void chip8_reset()
{
   memset(myChip8.ram, 0, sizeof myChip8.ram);
   memset(myChip8.V, 0, sizeof myChip8.V);
   memset(myChip8.stack, 0, sizeof myChip8.stack);
   myChip8.I = 0;
   myChip8.PC = PROGRAM_START;
   myChip8.sp = 0;
   myChip8.delay_timer = 0;
   myChip8.sound_timer = 0;
}

int chip8_load_rom(const char* const file_path) 
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
   // fetch 16 bit opcode
   uint16_t opcode = myChip8.ram[myChip8.PC];     // grab first 8 bits of opcode
   opcode = opcode << 8;                          
   opcode = opcode | myChip8.ram[myChip8.PC + 1]; // bitwise or the first 8 bits with second 8 bits to form 16 bit opcode

   // first 4 bits (nibble) of a opcode
   uint8_t first_nibble = (opcode & 0xF000) >> 12;
   printf("%04x %04x ", myChip8.PC, opcode);

   // increment program counter to point to next intruction (next 2 bytes)
   myChip8.PC += 2;
   
   switch(first_nibble)
   {
      case 0x0: 
      {
         if (opcode == 0x00E0)
         {
            display_clear();
            printf("00E0 opcode\n");
         }
         else if (opcode == 0x00EE)
         {
            if (myChip8.sp > 0)
            {
               myChip8.sp -= 1;
               myChip8.PC = myChip8.stack[myChip8.sp]; // return from subroutine, jump to return address
            }
            else
            {
               printf("Empty stack, cannont return!\n");
            }

            printf("00EE opcode\n");
         }
         break;
      } 
      case 0x1: 
      {
         uint16_t NNN = opcode & 0x0FFF;
         myChip8.PC = NNN; // jump to address NNN

         printf("%01x %03x\n", first_nibble, NNN); 
         break;
      }
      case 0x2: 
      {
         uint16_t NNN = opcode & 0x0FFF;

         myChip8.stack[myChip8.sp] = myChip8.PC; // save address of next opcode onto the stack (return address)

         if (myChip8.sp < MAX_STACK_LEVEL + 1)
         {
            myChip8.sp += 1;
            myChip8.PC = NNN; // execute subroutine at address NNN
         }
         else 
         {
            printf("Stack overflow occured!\n");
         }

         printf("%01x %03x\n", first_nibble, NNN); 
         break;
      }
      case 0x3: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t NN = opcode & 0x00FF;

         if (myChip8.V[X] == NN)
         {
            myChip8.PC += 2; // skip next intruction if V[X] equals NN
         }

         printf("%01x %01x %02x\n", first_nibble, X, NN); 
         break;
      }
      case 0x4: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t NN = opcode & 0x00FF;

         if (myChip8.V[X] != NN)
         {
            myChip8.PC += 2; // skip next instruction if V[X] not equals NN
         }

         printf("%01x %01x %02x opcode\n", first_nibble, X, NN); 
         break;
      }
      case 0x5: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t Y = (opcode & 0x00F0) >> 4;

         if (myChip8.V[X] == myChip8.V[Y])
         {
            myChip8.PC += 2; // skip next instruction if V[X] equals V[Y]
         }

         printf("%01x %01x %01x opcode\n", first_nibble, X, Y);
         break;
      }
      case 0x6: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t NN = opcode & 0x00FF;

         myChip8.V[X] = NN; // load register V[X] with 8 bit immediate NN

         printf("%01x %01x %02x opcode\n", first_nibble, X, NN); 
         break;
      }
      case 0x7: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint16_t NN = opcode & 0x00FF;

         myChip8.V[X] += NN; // add 8 bit immediate to register V[X]

         printf("%01x %01x %02x opcode %d + %d = %d\n", first_nibble, X, NN, X, NN, X + NN); 
         break;
      }
      case 0x8: 
      {
         uint8_t last_nibble = opcode & 0x000F;
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t Y = opcode & 0x00F0 >> 4;

         if (last_nibble == 0x0)
         {
            myChip8.V[X] = myChip8.V[Y]; // store V[Y] into V[X]
         }
         else if (last_nibble == 0x1)
         {
            myChip8.V[X] = myChip8.V[X] | myChip8.V[Y]; // set V[X] to biwize or of V[X] and V[Y]
         }
         else if (last_nibble == 0x2)
         {
            myChip8.V[X] = myChip8.V[X] & myChip8.V[Y]; // set V[X] to biwize and of V[X] and V[Y]
         }
         else if (last_nibble == 0x3)
         {
            // to XOR two bit patterns get results of bitwise AND and bitwise NOR
            // then NOR these two results together to get the XOR output

            uint8_t AND = myChip8.V[X] & myChip8.V[Y];
            uint8_t NOR = ~ ( myChip8.V[X] | myChip8.V[Y] ); // a bitwise NOR is the negated output of a bitwise OR

            // bitwise NOR the previous AND and NOR outputs
            uint8_t XOR_output = ~ ( AND | NOR );

            myChip8.V[X] = XOR_output; // store VX XOR VY into VX
         }
         else if (last_nibble == 0x4)
         {
            // set register VF to 1 on overflow, otherwise set to 0
            myChip8.V[0xF] = ( myChip8.V[X] + myChip8.V[Y] ) > 255;

            myChip8.V[X] += myChip8.V[Y]; // add VY to VX, will wrap on overflow because registers are unsigned
         }
         else if (last_nibble == 0x5)
         {

         }
         else if (last_nibble == 0x6)
         {

         }
         else if (last_nibble == 0x7)
         {

         }
         else if (last_nibble == 0xE)
         {
            
         }

         printf("8 opcode\n"); 
         break;
      }
      case 0x9: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t Y = (opcode & 0x00F0) >> 4;

         if (myChip8.V[X] != myChip8.V[Y])
         {
            myChip8.PC += 2; // skip next instruction if V[X] not equals V[Y]
         }

         printf("%01x %01x %01x opcode\n", first_nibble, X, Y); 
         break;
      }
      case 0xA: 
      {
         uint16_t NNN = opcode & 0x0FFF;

         myChip8.I = NNN; // load address register with address NNN
         
         printf("%01x %03x opcode\n", first_nibble, NNN);
         break;
      }
      case 0xB: 
      {
         uint16_t NNN = opcode & 0x0FFF;

         myChip8.PC = NNN + myChip8.V[0];

         printf("%01x %03x opcode\n", first_nibble, NNN); 
         break;
      }
      case 0xC: printf("C opcode\n"); break;
      case 0xD: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t Y = (opcode & 0x00F0) >> 4;
         uint8_t N = (opcode & 0x000F);
 
         myChip8.V[X] = myChip8.V[X] % PIXELS_W;
         myChip8.V[Y] = myChip8.V[Y] % PIXELS_H;

         printf("%01x %01x %01x %01x opcode\n", first_nibble, X, Y, N); 
         display_draw(myChip8.V[X], myChip8.V[Y], N);
         
         break;
      }
      case 0xE: printf("E opcode\n"); break;
      case 0xF: printf("F opcode\n"); break;
      default: printf("Invalid opcode encountered! %04x\n", opcode);
   }
}
