#include <stack>
#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include <SDL3/SDL.h>
#include "chip8.h"

// Memory -> 4KB of RAM. ()
// Display -> 64 x 32 pixels. Not a part of CPU; But maybe here anyways
// Program Counter -> points to current instruction in memory (12 bits)
// Index Register 'I' that points to locations in memory
// Stack for 16 bit adresses; Used to call functions and return from them 
// 8 bit delay timer; Decremented at 60Hz until it reaches 0
// 8 bit sound timer; beeping sound as long as value is > 0; Decrements at 60Hz as well.
// 16 1 byte variable registers; labelled 0 -> F or 0 to 15 in decimal; called V0 - VF
// VF -> flag register; used depending on instruction. 
// How to structure ? Have a struct for the state, and then various methods on that struct ?

struct cpu_state {
	uint8_t delay_timer; 
	uint8_t sound_timer; 
	uint16_t I; // Index register 
	uint16_t pc; // Program counter. Technically only needs 12 bits.  
	uint8_t v[16]; // v0 -> vf registers 
	uint8_t memory[CHIP8_RAM]; // RAM; All memory is editable 
	std::stack<uint16_t> st; // stack for functions and such; Calls functions at that address. 
}; 

CHIP8::CHIP8(char* codebuffer, int length) {
	// Turn on the cpu and set last step time to current time.
	last_cpu_time = SDL_GetTicksNS();
	power = true; 
	// Preffered initalizer list initalization.
	chip8_state = new cpu_state{}; 
	// Font data; Set in the first 512 bytes (0 -> 1FF). 
	// Should be in 050 - 09F. 
	uint8_t font_data[] = {
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
	
	// Begins at adress 50 in Hex, or 80 in decimal 
	std::memcpy(&(chip8_state->memory[FONT_ADDRESS]), font_data, FONT_ADDRESS);
	// Copy the program code to address 200 in memory 
	std::memcpy(&(chip8_state->memory[PROGRAM_ADDRESS]), codebuffer, length);
	chip8_state->pc = PROGRAM_ADDRESS; // Set program counter to program location. 		
}

uint16_t CHIP8::fetch_instruction() {
	// an instruction is 16 bits; 
	// the Program counter points to the
	// first 8 bits, 
	uint16_t curr_pc = chip8_state->pc; 

	if(curr_pc >= CHIP8_RAM) {
		printf("Pointer %x out of bounds!\n", curr_pc);
		error(); 
		return 0;
	}

	uint8_t high = chip8_state->memory[curr_pc]; // First part of instruction
	uint8_t low = chip8_state->memory[curr_pc + 1]; // Second part of instruction
	chip8_state->pc += 2; // Move to next instruction
	// Now we need to combine the 2 parts of the instruction; 
	// Shift the high part 8 pits to the right 
	return (static_cast<uint16_t>(high) << 8) | (static_cast<uint16_t>(low)); 
}

void CHIP8::error() {
	power = false; 
}; 

bool CHIP8::is_on() {
	return power; 
}

bool CHIP8::display_bit(int i, int j) {
	return display[i][j];	
}

void CHIP8::execute_instruction(uint16_t inst) {
	// printf("Executing Instruction: %04x\n", inst); 
		
	auto unimplemented_instruction = [&](uint16_t inst) {
		printf("Instruction %04x not implemented!\n", inst);
		error(); 
	}; 
	
	switch((inst >> 12)) {
		case 0x0: {
			// Instruction 00E0
			if(inst == 0x00e0) {
				// printf("Clearing screen...\n"); 	
				for(int i = 0; i < DISPLAY_HEIGHT; i++) {
					for(int j = 0; j < DISPLAY_WIDTH; j++) {
						display[i][j] = false; // Set each bit to OFF.		
					}
				}
			// Instruction 00EE
			} else if(inst == 0x00ee) {
				// Return instruction; set PC to top address, then
				// pop from stack. 
				if(chip8_state->st.empty()) {
					printf("Error! Somehow return is called, but no return address exists in stack");
					error(); 
				} else {
					chip8_state->pc = chip8_state->st.top(); 
					chip8_state->st.pop(); 
				}
			} else {
				unimplemented_instruction(inst); 	
			}
		} break; 
		// Instruction 1NNN
		case 0x1: {
			uint16_t NNN = (inst & 0x0FFF); 
			chip8_state->pc = NNN; 	
			// printf("Jump to adress: %x\n", jump); 
		} break;
		// Instruction 2NNN
		case 0x2: {
			// Execute function at NNN; 
			// Essentially, Save current PC in stack, then Jump to NNN
			uint16_t NNN = (inst & 0x0FFF); 	
			chip8_state->st.push(chip8_state->pc);
			chip8_state->pc = NNN;
		} break; 
		// Instruction 6XNN
		case 0x6: {
			// Second nibble is the register. 
			uint8_t x = (inst & 0x0F00) >> 8;
			// Last byte is the value we need to set. 
			uint8_t NN = static_cast<uint8_t>(inst); 
			// Set corresponding register. 
			chip8_state->v[x] = NN; 		
			// printf("Setting Register %x, with %x\n", x, NN); 
		} break; 
		// Instruction 7XNN
		case 0x7: {
			// Second nibble is the register. 
			uint8_t x = (inst & 0x0F00) >> 8;
			// Last byte is the value we need to set. 
			uint8_t NN = static_cast<uint8_t>(inst); 
			// add to corresponding register. 
			chip8_state->v[x] += NN; 		
			// printf("Setting Register %x, with %x\n", x, NN); 
		} break; 
		case 0xA: {
			uint16_t NNN = (inst & 0x0FFF); 
			chip8_state->I = NNN; 
			// printf("Setting I register with %x\n", NNN); 
		} break; 
		// Instruction DXYN
		case 0xD: {
			// Second Nibble is the X register. 
			uint8_t x = (inst & 0x0F00) >> 8; 
			// Third Nibble is the Y register. 
			uint8_t y = (inst & 0x00F0) >> 4; 
			// Last Nibble is N;
			uint8_t N = (inst & 0x000F);
			
			// x and y coordinates are modded
			int x_coord = chip8_state->v[x] % DISPLAY_WIDTH; 
			int y_coord = chip8_state->v[y] % DISPLAY_HEIGHT; 	
			
			// Sprite is stored starting from location I register is pointing to
			uint16_t sprite_addr = chip8_state->I;
			
			// start at x_coord, and y_coord. 
			// printf("Drawing sprite to screen...\n", N);
			
			// For each y, draw the row x. 
			// 10000000 = 0x80. Shift by k + 1, to the left.  
			for(int i = y_coord; i < y_coord + N && i < DISPLAY_HEIGHT; i++) {
				uint8_t sprite_layer = chip8_state->memory[sprite_addr];
				for(int j = x_coord, k = 0; k < 8 && j < DISPLAY_WIDTH; j++, k++) {
					// if set bit is unset, set VF register to 1. 
					// Otherwise, set VF to 0.
					// bit is xoord with display bit.
					bool last_bit = display[i][j]; 
					display[i][j] ^= ((sprite_layer & (0x80 >> k)) >> (8 - (k + 1))); 
					if(last_bit && last_bit != display[i][j]) {
						chip8_state->v[0xF] = 1;
					} else {
						chip8_state->v[0xF] = 0;
					}
				}
				sprite_addr++; 
			}
		} break; 
		default: {
			unimplemented_instruction(inst);
		}
	}
}

void CHIP8::CPU_step() {
	// get the last CPU_step() time. 
	uint64_t time_step = SDL_GetTicksNS() - last_cpu_time;
	// Now, we need to convert this to seconds, (/1e9), then Multiply by the 
	// # of instructions per second to calculate how many CPU cycles should have
	// passed in between the 2 times. 
	// variable time step is OK since this is not a physics simulation
	
	for(uint64_t i = 0; i < (time_step * INSTRUCTIONS_PER_SECOND) / 1000000000; i++) {
		if(!power) {
			continue;
		}
		uint16_t curr_inst = fetch_instruction(); 
		execute_instruction(curr_inst); 
	}
	last_cpu_time = SDL_GetTicksNS();
}

CHIP8::~CHIP8() {
	delete chip8_state; 
}
