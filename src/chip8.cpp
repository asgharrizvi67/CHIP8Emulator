#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
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
	// seed random number generator
	srand(time(NULL)); 
}

uint16_t CHIP8::fetch_instruction() {
	// an instruction is 16 bits; 
	// the program counter points to the
	// first 8 bits, 
	uint16_t curr_pc = chip8_state->pc; 

	if(curr_pc >= CHIP8_RAM) {
		printf("Pointer %x out of bounds!\n", curr_pc);
		error(); 
		return 0;
	}

	uint8_t high = chip8_state->memory[curr_pc]; // First part of instruction (CHIP8 is big endian)
	uint8_t low = chip8_state->memory[curr_pc + 1]; // Second part of instruction
	chip8_state->pc += 2; // Move to next instruction
	// Now we need to combine the 2 parts of the instruction; 
	// Shift the high part 8 bits to the right 
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
		//error(); 
	}; 
	
	switch((inst >> 12)) {
		case 0x0: {
			if(inst == 0x00E0) {
				// printf("Clearing screen...\n"); 	
				for(int i = 0; i < DISPLAY_HEIGHT; i++) {
					for(int j = 0; j < DISPLAY_WIDTH; j++) {
						display[i][j] = false; // Set each bit to OFF.		
					}
				}
			} else if(inst == 0x00EE) {
				// Return instruction; set PC to top address, then
				// pop from stack. 
				if(chip8_state->st.empty()) {
					printf("Error! Somehow return is called, but no return address exists in stack");
					error(); 
				} else {
					chip8_state->pc = chip8_state->st.top(); 
					chip8_state->st.pop(); 
				}
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
		// Instruction 3XNN
		case 0x3: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t NN = static_cast<uint8_t>(inst);
			
			// Skip following instruction (increment pc by 2) if value of VX equals NN. 
			if(chip8_state->v[x] == NN) {
				chip8_state->pc += 2; 
			}
		} break; 
		// Instruction 4XNN
		case 0x4: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t NN = static_cast<uint8_t>(inst);
			
			// Skip following instruction (increment pc by 2) if value of VX does not equal NN. 
			if(chip8_state->v[x] != NN) {
				chip8_state->pc += 2; 
			}
		} break;
		// Instruction 5XY0
		case 0x5: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t y = (inst & 0x00F0) >> 4;
			
			// Skip following instruction (increment pc by 2) if value of VX equals VY
			if(chip8_state->v[x] == chip8_state->v[y]) {
				chip8_state->pc += 2; 
			}
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
		// Arithemtic Instructions
		case 0x8: {
			// Last Nibble determines which instruction it is.			
			// Instructions 8XYK where K >= 0
			switch(inst & 0x000F) {
				case 0x0: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					// Store the value of VY in VX. 
					chip8_state->v[x] = chip8_state->v[y];
				} break; 
				
				case 0x1: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					// VX is set to VX | VY. 
					chip8_state->v[x] |= chip8_state->v[y]; 
				} break;  
				case 0x2: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					// VX is set to VX & VY. 
					chip8_state->v[x] &= chip8_state->v[y];
				} break; 
				case 0x3: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					// VX is set to VX ^ VY. 
					chip8_state->v[x] ^= chip8_state->v[y];
				} break; 
				case 0x4: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					// VX is set to VX + VY. 
					// If VX + VY overflows (Carry), 
					// then set the VF flag to 1.
					// max value of uint8_t is 
					// 255 (2^8 - 1) . if (VX + VY) > 255 or
					// VX > 255 - VY without overflow. 
					
					if(chip8_state->v[x] > UINT8_MAX - chip8_state->v[y]) {
						chip8_state->v[0xF] = 1; 	
					} else {
						chip8_state->v[0xF] = 0; 
					}

					chip8_state->v[x] += chip8_state->v[y]; 
				} break;
				case 0x5: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					// VX is set to VX - VY. 
					// If Borrow occurs, VF = 0. 
					// If Borrow does not occur VF = 1 
					// if VX - VY < 0 -> underflow; VF = 0. (No carry). 
					// (Left flag is still 1, meaning it should be 
					// negative, but since it's unsigned there is 
					// overflow of two's complement. 
					// Otherwise, the sign bit is 0 and there
					// is no underflow, due to carry. 

					if(chip8_state->v[x] < chip8_state->v[y]) {
						chip8_state->v[0xF] = 0; 
					} else {
						chip8_state->v[0xF] = 1;	
					}
					
					chip8_state->v[x] -= chip8_state->v[y]; 
				} break; 
				// *************** AMBIGUOUS INSTRUCTION (8XY6 and 8XYE) ***************
				// On CHIP-48 and SUPER-CHIP, they no longer set 
				// VX to VY before the shift; Will make this configurable
				// later.
				case 0x6: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
						
					chip8_state->v[x] = chip8_state->v[y]; 
					
					// Shift right
					chip8_state->v[x] >>= 1; 
					// Set VF to LSB of VY. 
					chip8_state->v[0xF] = (chip8_state->v[y]) & 1; 
				} break; 
				case 0x7: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					// VX is set to VY - VX. 
					// If Borrow occurs, VF = 0. 
					// If Borrow does not occur VF = 1 
					// if VY - VX < 0 -> underflow; VF = 0. (No carry). 
					// (Left flag is still 1, meaning it should be 
					// negative, but since it's unsigned there is 
					// overflow of two's complement. 
					// Otherwise, the sign bit is 0 and there
					// is no underflow, due to carry. 

					if(chip8_state->v[y] < chip8_state->v[x]) {
						chip8_state->v[0xF] = 0; 
					} else {
						chip8_state->v[0xF] = 1;	
					}	
					
					chip8_state->v[x] = chip8_state->v[y] - chip8_state->v[x];
				} break;
				case 0xE: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4;
					
					// Optional step
					chip8_state->v[x] = chip8_state->v[y]; 
					// Shift left
					chip8_state->v[x] <<= 1;
					// Set VF to LSB of VY
					chip8_state->v[0xF] = (chip8_state->v[y]) & 1; 
				} break; 
				default: {
					unimplemented_instruction(inst); 
				}
			}
		} break; 
		// Instruction 9XY0
		case 0x9: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t y = (inst & 0x00F0) >> 4;
			
			// Skip following instruction (increment pc by 2) if value of VX does not equal VY
			if(chip8_state->v[x] != chip8_state->v[y]) {
				chip8_state->pc += 2; 
			}
		} break; 
		// Instruction ANNN
		case 0xA: {
			uint16_t NNN = (inst & 0x0FFF); 
			chip8_state->I = NNN; 
			// printf("Setting I register with %x\n", NNN); 
		} break; 
		// *************** AMBIGUOUS INSTRUCTION (BNNN) *************** 
		// CHIP-48 and SUPER-CHIP changed to BXNN, where it will jump to XNN + the value in v[x]. 
		case 0xB: {
			uint16_t NNN = (inst & 0x0FFF); 
			// Jump to address NNN + v[0]. 
			chip8_state->pc = NNN + chip8_state->v[0]; 
		} break; 
		// Instruction CXNN  
		case 0xC: {
			uint8_t x = (inst & 0xF00) >> 8; 
			uint8_t NN = (inst & 0x00FF);
			// Generate random number and binary AND with NN and put in VX. 
			uint8_t random = (rand() % (UINT8_MAX + 1)); // generates between 0 and max of 8 bit number 
			chip8_state->v[x] = random & NN; 
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
				if(sprite_addr >= CHIP8_RAM) {
					printf("Error! Index register out of bounds!");
					error(); 
					break;  
				}
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
		// Keyboard instructions; Do these later
		case 0xE: {
				
		} break; 
		case 0xF: {
			// Instruction determined by last byte. 
			uint8_t x = (inst & 0x0F00) >> 8; 	
			switch(inst & 0x00FF) {
				// Set VX to delay timer
				case 0x07: {
					chip8_state->v[x] = chip8_state->delay_timer; 	
				} break;
				// Wait for keypress; Do this laster
				case 0x0A: {
							
				} break;
				// Set delay timer to VX
				case 0x15: {
					chip8_state->delay_timer = chip8_state->v[x]; 
				} break; 
				// Set sound timer to VX
				case 0x18: {
					chip8_state->sound_timer = chip8_state->v[x]; 
				} break; 
				case 0x1E: {
					// Unsure how flag is set; 
					// In the other instructions, VF was set to 0
					// if there was no carry; So I will implement
					// this behaviour. 
					if(chip8_state->I >= CHIP8_RAM - chip8_state->v[x]) {
						chip8_state->v[0xF] = 1; 	
					} else {
						chip8_state->v[0xF] = 0;
					}
					chip8_state->I += chip8_state->v[x]; 	
				} break; 
				case 0x29: {
					// Font character; Only take last nibble
					uint8_t vx_char = chip8_state->v[x] & 0x000F; 
					// 0 -> address 80. 
					// 1 -> address 85. 
					// 2 -> address 90. 
					// Linear relationship, between vx_char and address.
					// y = 80 + 5 * vx_char. 
					chip8_state->I = 0x50 + (5 * vx_char); 
				} break; 
				case 0x33: {
					// Stores BCD of vx in I, I + 1 and I + 2. 
					uint8_t val = chip8_state->v[x]; 
					uint16_t addr = chip8_state->I;
					if(addr < CHIP8_RAM) {
						chip8_state->memory[addr] = (val / 100); // First digit
					} else {
						printf("Could not set BCD value! Address out of bounds!"); 
						error(); 
					}
					if(addr + 1 < CHIP8_RAM) {
						chip8_state->memory[addr + 1] = (val / 10) % 10; // Second digit 
					} else {
						printf("Could not set BCD value! Address out of bounds!"); 
						error(); 
					}
					if(addr + 2 < CHIP8_RAM) {
						chip8_state->memory[addr + 2] = val % 10; // Last digit
					} else {
						printf("Could not set BCD value! Address out of bounds!"); 
						error(); 
					}
				} break; 
				case 0x55: {
					uint16_t addr = chip8_state->I; 
					for(uint8_t i = 0; i <= x; i++) {
						if(addr >= CHIP8_RAM) {
							printf("Could not load memory! address out of bounds!");
							error(); 
							break;
						}
						chip8_state->memory[addr] = chip8_state->v[i]; 
						addr++; 
					}
					// *************** AMBIGUOUS INSTRUCTION *************** 
					// CHIP-48 and Super CHIP changed this so that I is not incremented to 
					// I + X + 1 after the operation. 
				} break; 
				case 0x65: {
					uint16_t addr = chip8_state->I; 
					for(uint8_t i = 0; i <= x; i++) {
						if(addr >= CHIP8_RAM) {
							printf("Could not store memory! address out of bounds!");
							error(); 
							break;
						}
						chip8_state->v[i] = chip8_state->memory[addr]; 
						addr++; 
					}
					// *************** AMBIGUOUS INSTRUCTION *************** 
					// CHIP-48 and Super CHIP changed this so that I is not incremented to 
					// I + X + 1 after the operation. 
				} break; 
				default: {
					unimplemented_instruction(inst);
				} 
			}
		} break; 
		default: {
			unimplemented_instruction(inst);
		}
	}
}

void CHIP8::CPU_step() {
	static uint64_t last_cpu_time = SDL_GetTicksNS(); 
	static uint64_t remainder = 0; 
	
	uint64_t curr_time = SDL_GetTicksNS(); 
	uint64_t time_step = curr_time - last_cpu_time;
	last_cpu_time = curr_time; 
	
	uint64_t s_to_ns = 1000000000;
	uint64_t cycles_per_frame = ((time_step * INSTRUCTIONS_PER_SECOND) / s_to_ns);
	remainder += (time_step * INSTRUCTIONS_PER_SECOND) - (cycles_per_frame * s_to_ns); 
	
	if(remainder >= s_to_ns) {
		remainder -= s_to_ns;
		cycles_per_frame++; 
	}
	
	for(uint64_t i = 0; i < cycles_per_frame; i++) {
		if(!power) {
			continue;
		}
		uint16_t curr_inst = fetch_instruction(); 
		execute_instruction(curr_inst); 
	}
}

void CHIP8::decrement_timers() { // Seperate method, since these need to be decrement independant of the CPU speed
	if(chip8_state->delay_timer > 0) {
		chip8_state->delay_timer--; 
	}
	if(chip8_state->sound_timer > 0) {
		chip8_state->sound_timer--; 
	}
}

void CHIP8::update_keystate() {
	keyboard_state = SDL_GetKeyboardState(NULL); 
}

CHIP8::~CHIP8() {
	delete chip8_state; 
}
