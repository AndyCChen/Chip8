#ifndef CHIP8_H
#define CHIP8_H 

#include <stdbool.h>

/*
	max size in bytes for chip8 ram
	addresses 0x000 - 0x1FF are reserved
*/
#define RAM_SIZE 4096

// programs are loaded into address 0x200 (byte 512)
#define PROGRAM_START 0x200

// starting address in ram to load the built in font
#define FONT_START 0x050

// number of general purpose registers
#define V_REGISTERS 16

// maximum number of stack levels
#define MAX_STACK_LEVEL 16

typedef struct {
	uint8_t ram[RAM_SIZE];
	uint8_t V[V_REGISTERS];
	uint16_t I;
	uint16_t PC;
	uint16_t stack[MAX_STACK_LEVEL];
	uint8_t sp;
	uint8_t delay_timer;
	uint8_t sound_timer;
} Chip8;

extern Chip8 myChip8;

/* 
	resets all chip8 registers and timers
	also loads in the font at address 0x050
*/
void chip8_reset();

/**
 * load program into chip8 memory at location 0x200 (byte 512)
 * returns number of bytes read
 * returns 0 on error
*/
int chip8_load_rom(const char* const);

// a single cycle to fetch, decode, and execute one instruction
void chip8_run_cycle(bool log_flag);

// decrements delay timer at 60hz when it is non zero
void chip8_decrement_delay_timer();

// decrements sound timer at 60hz when it is non zero
void chip8_decrement_sound_timer();

// sets a key to be in the pressed state
void chip8_set_key_down(uint8_t key);

// sets a key to be in the released state
void chip8_set_key_up(uint8_t key);


#endif