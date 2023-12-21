#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "../includes/chip8.h"
#include "../includes/display.h"

// max length of the disassembler log buffer
#define DSAM_LOG_SIZE 255

// string containing info on what instruction is being executed in the current cycle
static char disasembler_log[DSAM_LOG_SIZE];

// global chip 8 instance
Chip8 myChip8;

/* holds the states of the 15 key on the keypad
   each bit coresponds to a key that is in the pressed or released state
   0: released
   1: pressed
*/
static uint16_t keypad = 0;

// holds the hex value of the key that was last pressed
static uint8_t pressed_key;

// flag to check if a key was released in previous frame
static bool is_key_released = false;

// fonts representing the numbers 0x0 - 0xF
static uint8_t fonts[] = 
{
   0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
   0x20, 0x60, 0x20, 0x20, 0x70, // 1
   0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
   0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
   0x90, 0x90, 0xF0, 0x10, 0x10, // 4
   0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
   0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
   0xF0, 0x10, 0x20, 0x40, 0x40, // 7
   0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
   0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
   0xF0, 0x90, 0xF0, 0x90, 0x90, // A
   0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
   0xF0, 0x80, 0x80, 0x80, 0xF0, // C
   0xE0, 0x90, 0x90, 0x90, 0xE0, // D
   0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
   0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

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

   // load the font into address 0x050 in ram
   memcpy(&myChip8.ram[FONT_START], fonts, sizeof fonts);
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

void chip8_run_cycle(bool log_flag)
{
   // fetch 16 bit opcode
   uint16_t opcode = myChip8.ram[myChip8.PC];     // grab first 8 bits of opcode
   opcode = opcode << 8;                          
   opcode = opcode | myChip8.ram[myChip8.PC + 1]; // bitwise or the first 8 bits with second 8 bits to form 16 bit opcode

   // first 4 bits (nibble) of a opcode
   uint8_t first_nibble = (opcode & 0xF000) >> 12;
   int dsam_log_offset = snprintf(disasembler_log, DSAM_LOG_SIZE, "%04x %04x ", myChip8.PC, opcode);

   // increment program counter to point to next intruction (next 2 bytes)
   myChip8.PC += 2;
   
   switch(first_nibble)
   {
      case 0x0: 
      {
         if (opcode == 0x00E0)
         {
            display_clear();
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "00E0 CLEAR SCREEN\n");
         }
         else if (opcode == 0x00EE)
         {
            if (myChip8.sp > 0)
            {
               myChip8.sp -= 1;
               myChip8.PC = myChip8.stack[myChip8.sp]; // return from subroutine, jump to return address
               snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "00EE RETURN: %04x from subroutine\n", myChip8.PC);
            }
            else
            {
               snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "Empty stack, cannont return!\n");
            }
         }
         break;
      } 
      case 0x1: 
      {
         uint16_t NNN = opcode & 0x0FFF;
         myChip8.PC = NNN; // jump to address NNN

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "1NNN JUMP to NNN: %03x\n", NNN); 
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
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "Stack overflow occured!\n");
         }

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "2NNN Call Subroutine at NNN: %03x\n", NNN); 
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

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "3XNN SKIP if VX: %d == NN %d\n", myChip8.V[X], NN); 
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

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "4XNN SKIP if VX: %d != NN: %d\n", myChip8.V[X], NN); 
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

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "5XY0 SKIP if VX: %d == VY: %d\n", myChip8.V[X], myChip8.V[Y]);
         break;
      }
      case 0x6: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t NN = opcode & 0x00FF;

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "6XNN LOAD VX: %d with NN: %d\n", myChip8.V[X], NN); 
         myChip8.V[X] = NN; // load register V[X] with 8 bit immediate NN

         break;
      }
      case 0x7: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint16_t NN = opcode & 0x00FF;

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "7XNN ADD NN: %d into VX: %d\n", NN, myChip8.V[X]); 
         myChip8.V[X] += NN; // add 8 bit immediate to register V[X]
 
         break;
      }
      case 0x8: 
      {
         uint8_t last_nibble = opcode & 0x000F;
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t Y = (opcode & 0x00F0) >> 4;

         if (last_nibble == 0x0)
         {
            myChip8.V[X] = myChip8.V[Y]; // store V[Y] into V[X]
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XY0 MOV VY: %d into VX\n", myChip8.V[Y]);
         }
         else if (last_nibble == 0x1)
         {
            myChip8.V[X] = myChip8.V[X] | myChip8.V[Y]; // set V[X] to biwize or of V[X] and V[Y]
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XY1 OR VX VY\n");
         }
         else if (last_nibble == 0x2)
         {
            myChip8.V[X] = myChip8.V[X] & myChip8.V[Y]; // set V[X] to biwize and of V[X] and V[Y]
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XY2 AND VX VY\n");
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
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XY3 XOR VX VY\n");
         }
         else if (last_nibble == 0x4)
         {
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XY4 ADD VX: %d VY: %d\n", myChip8.V[X],  myChip8.V[Y]); 

            uint8_t  first_operand = myChip8.V[X]; // save value of VX for overflow check later

            // add VY to VX, will wrap on overflow because registers are unsigned
            myChip8.V[X] += myChip8.V[Y]; 
            
            // set register VF to 1 on overflow, otherwise set to 0
            myChip8.V[0xF] = ( first_operand + myChip8.V[Y]) > 255; 
         }
         else if (last_nibble == 0x5)
         {
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XY5 SUB VX: %d VY: %d\n", myChip8.V[X], myChip8.V[Y]);

            uint8_t minuend = myChip8.V[X];
            uint8_t subtrahend = myChip8.V[Y];

            // V[X] = V[X] - V[Y]
            myChip8.V[X] = minuend - subtrahend;
            // set register V[F] to 1 if V[X] >= V[Y]
            myChip8.V[0xF] = ( minuend >= subtrahend );
         }
         else if (last_nibble == 0x6)
         {
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XY6 RIGHT SHIFT 1 bit VY INTO VX\n");

            uint8_t VY_temp = myChip8.V[Y];

            // right shift V[Y] by 1 bit and store result into V[X]
            myChip8.V[X] = myChip8.V[Y] >> 1;

            // store least significant bit of V[Y] into V[F]
            myChip8.V[0xF] = VY_temp & 1 ;
         }
         else if (last_nibble == 0x7)
         { 
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XY7 SUB VY: %d VX: %d\n", myChip8.V[Y], myChip8.V[X]);

            uint8_t minuend = myChip8.V[Y];
            uint8_t subtrahend = myChip8.V[X];

            // V[X] = V[Y] - V[X]
            myChip8.V[X] = minuend - subtrahend;

            // set register V[F] to 1 if V[Y] >= V[X]
            myChip8.V[0xF] = minuend >= subtrahend;
         }
         else if (last_nibble == 0xE)
         {
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "8XYE RIGHT SHIFT 1 bit VY INTO VX\n");

            uint8_t VY_temp = myChip8.V[Y];

            // left shift V[Y] by 1 bit and store result into V[X]
            myChip8.V[X] = myChip8.V[Y] << 1;

            // store most significant bit of V[Y] into V[F]
            myChip8.V[0xF] = ( VY_temp & (1 << 7) ) >> 7;
         }

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

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "9XY0 SKIP IF VX: %d != VY: %d\n", myChip8.V[X], myChip8.V[Y]); 
         break;
      }
      case 0xA: 
      {
         uint16_t NNN = opcode & 0x0FFF;

         myChip8.I = NNN; // load address register with address NNN
         
         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "ANNN LOAD I REG with NNN: %03x\n", NNN);
         break;
      }
      case 0xB: 
      {
         uint16_t NNN = opcode & 0x0FFF;

         myChip8.PC = NNN + myChip8.V[0];

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "BNNN JUMP TO NNN: %03x + V0: %d\n", NNN, myChip8.V[0]); 
         break;
      }
      case 0xC: 
      {
         uint8_t X = ( opcode & 0x0F00 ) >> 8;
         uint8_t NN = ( opcode & 0x00FF );

         int random_number = rand();
         myChip8.V[X] = random_number & NN;        

         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "CXNN RAND NUM: %d\n", myChip8.V[X]); 
         break;
      }
      case 0xD: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t Y = (opcode & 0x00F0) >> 4;
         uint8_t N = (opcode & 0x000F);

         display_draw(myChip8.V[X] % PIXELS_W, myChip8.V[Y] % PIXELS_H, N);
         snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "DXYN DRAW SPRITE AT VX: %d, VY: %d, N: %d pixels high\n", myChip8.V[X] % PIXELS_W, myChip8.V[Y] % PIXELS_H, N); 

         break;
      }
      case 0xE: 
      {
         uint8_t last_two_nibble = opcode & 0x00FF;
         uint8_t X = ( opcode & 0x0F00 ) >> 8;

         uint16_t mask = 1 << ( myChip8.V[X] );
         uint8_t key = ( keypad & mask ) >> ( myChip8.V[X] );

         if (last_two_nibble == 0x9E)
         {
            // skip next instruction if key with the hex value in V[X] is pressed
            if (key == 1) myChip8.PC += 2;
            is_key_released = false;
            
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "EX9E SKIP IF key %1x is pressed\n", myChip8.V[X]);
         }
         else if (last_two_nibble == 0xA1)
         {
            // skip next instruction if key with the hex value in V[X] is not pressed
            if (key == 0) myChip8.PC += 2;
            is_key_released = false;

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "EXA1 SKIP IF key %1x not pressed\n", myChip8.V[X]);
         }

         break;
      }
      case 0xF: 
      {
         uint8_t X = (opcode & 0x0F00) >> 8;
         uint8_t last_two_nibble = opcode & 0x00FF;

         if (last_two_nibble == 0x07)
         {
            myChip8.V[X] = myChip8.delay_timer;
            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX07 LOAD delay_timer: %d into VX\n", myChip8.delay_timer);
         }
         else if (last_two_nibble == 0x0A)
         {
            // wait for key release and store released key in VX
            // when keypad is zero it means no keys are being pressed
            // so we decrement program counter to wait for a key press again
            if ( !is_key_released ) myChip8.PC -= 2;
            else 
            {
               myChip8.V[X] = pressed_key; // else we set V[X] to the key that last was pressed
               is_key_released = false;    // reset flag back to false
            }

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX0A WAIT for keypress and store in VX\n");
         }
         else if (last_two_nibble == 0x15)
         {
            // set delay timer to value of register V[X]
            myChip8.delay_timer = myChip8.V[X];

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX15 SET delay timer to value in VX: %d\n", myChip8.V[X]);
         }
         else if (last_two_nibble == 0x18)
         {
            // set sound timer to value of register V[X]
            myChip8.sound_timer = myChip8.V[X];

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX18 SET sound timer to value in VX: %d\n", myChip8.V[X]);
         }
         else if (last_two_nibble == 0x1E)
         {
            // add value in register V[X] to register I
            myChip8.I += myChip8.V[X];

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX1E ADD VX: %d to register I\n", myChip8.V[X]);
         }
         else if (last_two_nibble == 0x29)
         {
            // set I to point to the font sprite corresponding to the hex value in V[X]
            // multiply by 5 because fonts are 5 pixels high
            myChip8.I = FONT_START + ( 5 * myChip8.V[X] );

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX29 SET register I to point to font with hex value in VX: %d\n", myChip8.V[X]);
         }
         else if (last_two_nibble == 0x33)
         {
            // store the binary coded decimal of value in V[X] at: I, I + 1, I + 2
            uint8_t decimal = myChip8.V[X];

            uint8_t ones = decimal % 10;
            uint8_t tens = ( ( decimal - ones ) % 100 ) / 10;
            uint8_t hundreds =  ( ( decimal - ones ) - ( ( decimal - ones ) % 100 ) ) / 100;

            myChip8.ram[myChip8.I] = hundreds;
            myChip8.ram[myChip8.I + 1] = tens;
            myChip8.ram[myChip8.I + 2] = ones;

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX33 STORE BCD %d %d %d of VX: %d into address starting add reg I\n", hundreds, tens, hundreds, myChip8.V[X]);
         }
         else if (last_two_nibble == 0x55)
         {
            // load registers V[0] - V[X] into memory starting at address I
            for (int index = 0; index <= X; ++index)
            {
               myChip8.ram[myChip8.I + index] = myChip8.V[index];
            }

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX55 LOAD V0 to VX into memory starting at address I\n");
         }
         else if (last_two_nibble == 0x65)
         {
            // load values from memory starting at address I into registers V[0] - V[X]
            for (int index = 0; index <= X; ++index)
            {
               myChip8.V[index] = myChip8.ram[myChip8.I + index];
            }

            snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "FX65 LOAD values starting from address I into registers V0 to VX\n");
         }
   
         break;
      };
      default: snprintf(disasembler_log + dsam_log_offset, DSAM_LOG_SIZE, "Invalid opcode encountered!\n");
   }

   // decrement timers every cycle
   chip8_update_timers();

   if (log_flag) printf("%s", disasembler_log);
}

void chip8_set_key_down(uint8_t key)
{
   keypad = keypad | ( 1 << key );
   pressed_key = key;
}

void chip8_set_key_up(uint8_t key)
{
   keypad = keypad & ~( 1 << key );
   is_key_released = true;
}

void chip8_update_timers()
{
   static float dt = 0;
   static clock_t current_time, previous_time = 0;

   current_time = clock();
   dt += (float) ( current_time - previous_time ) / CLOCKS_PER_SEC;
   previous_time = current_time;

   while (dt >= (float) 1 / 60)
   {
      if (myChip8.delay_timer > 0) myChip8.delay_timer -= 1;
      if (myChip8.sound_timer > 0) myChip8.sound_timer -= 1;

      // play beep audio when sound timer is not zero
      if (myChip8.sound_timer > 0) 
         display_pause_audio_device(0);
      else 
         display_pause_audio_device(1);

      dt -= (float) 1 / 60;
   }
}