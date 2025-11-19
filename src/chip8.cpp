
#include "chip8.h"
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <cstring>
#include <SDL3/SDL.h>

CHIP8::CHIP8(char* codebuffer, int length) : chip8_state{}, keyboard{} {
	power = true; 
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
	std::memcpy(&(chip8_state.memory[FONT_ADDRESS]), font_data, FONT_ADDRESS);
	// Copy the program code to address 200 in memory 
	std::memcpy(&(chip8_state.memory[PROGRAM_ADDRESS]), codebuffer, length);
	chip8_state.pc = PROGRAM_ADDRESS; // Set program counter to program location. 		
	
	srand(time(NULL)); 
}

uint16_t CHIP8::fetch_instruction() {
	uint16_t curr_pc = chip8_state.pc; 
	if(curr_pc >= CHIP8_RAM) {
		printf("Pointer %x out of bounds!\n", curr_pc);
		error(); 
		return 0;
	}

	uint8_t high = chip8_state.memory[curr_pc]; // First part of instruction (CHIP8 is big endian)
	uint8_t low = chip8_state.memory[curr_pc + 1]; // Second part of instruction
	chip8_state.pc += 2; // Move to next instruction
	
	return (static_cast<uint16_t>(high) << 8) | (static_cast<uint16_t>(low)); 
}

void CHIP8::error() {
	power = false; 
}; 

bool CHIP8::is_on() {
	return power; 
}

bool CHIP8::sound() {
	return chip8_state.sound_timer > 0;
}

bool CHIP8::display_bit(int i, int j) {
	return display[i][j];	
}

void CHIP8::execute_instruction(uint16_t inst) {
	auto unimplemented_instruction = [&](uint16_t inst) {
		printf("Instruction %04x not implemented!\n", inst);
	}; 
	
	switch((inst >> 12)) {
		case 0x0: {
			if(inst == 0x00E0) {
				for(int i = 0; i < DISPLAY_HEIGHT; i++) {
					for(int j = 0; j < DISPLAY_WIDTH; j++) {
						display[i][j] = false; 		
					}
				}
			} else if(inst == 0x00EE) {
				// Return instruction; set PC to top address, then
				// pop from stack. 
				if(chip8_state.st.empty()) {
					printf("Error! Somehow return is called, but no return address exists in stack");
					error(); 
				} else {
					chip8_state.pc = chip8_state.st.top(); 
					chip8_state.st.pop(); 
				}
			}
		} break; 
		case 0x1: {
			uint16_t NNN = (inst & 0x0FFF); 
			chip8_state.pc = NNN; 	
		} break;
		case 0x2: {
			// Save current PC in stack, then Jump to NNN
			uint16_t NNN = (inst & 0x0FFF); 	
			chip8_state.st.push(chip8_state.pc);
			chip8_state.pc = NNN;
		} break;
		case 0x3: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t NN = static_cast<uint8_t>(inst);
			
			// Skip following instruction (increment pc by 2) if value of VX equals NN. 
			if(chip8_state.v[x] == NN) {
				chip8_state.pc += 2; 
			}
		} break; 
		case 0x4: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t NN = static_cast<uint8_t>(inst);
			
			// Skip following instruction (increment pc by 2) if value of VX does not equal NN. 
			if(chip8_state.v[x] != NN) {
				chip8_state.pc += 2; 
			}
		} break;
		case 0x5: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t y = (inst & 0x00F0) >> 4;
			
			// Skip following instruction (increment pc by 2) if value of VX equals VY
			if(chip8_state.v[x] == chip8_state.v[y]) {
				chip8_state.pc += 2; 
			}
		} break; 
		case 0x6: {
			uint8_t x = (inst & 0x0F00) >> 8;
			uint8_t NN = static_cast<uint8_t>(inst); 
			
			chip8_state.v[x] = NN; 		
		} break; 
		case 0x7: {
			uint8_t x = (inst & 0x0F00) >> 8;
			uint8_t NN = static_cast<uint8_t>(inst); 
			
			chip8_state.v[x] += NN; 		
		} break; 
		case 0x8: {
			switch(inst & 0x000F) {
				case 0x0: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					
					chip8_state.v[x] = chip8_state.v[y];
				} break; 
				
				case 0x1: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					
					chip8_state.v[x] |= chip8_state.v[y]; 
				} break;  
				case 0x2: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					
					chip8_state.v[x] &= chip8_state.v[y];
				} break; 
				case 0x3: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					
					chip8_state.v[x] ^= chip8_state.v[y];
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
					uint8_t VX = chip8_state.v[x]; 
					uint8_t VY = chip8_state.v[y]; 

					chip8_state.v[x] += chip8_state.v[y]; 
					if(VX > UINT8_MAX - VY) {
						chip8_state.v[0xF] = 1; 	
					} else {
						chip8_state.v[0xF] = 0; 
					}
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
					uint8_t VX = chip8_state.v[x]; 
					uint8_t VY = chip8_state.v[y]; 

					chip8_state.v[x] -= chip8_state.v[y]; 

					if(VX < VY) {
						chip8_state.v[0xF] = 0; 
					} else {
						chip8_state.v[0xF] = 1;	
					}
				} break; 
				// *************** AMBIGUOUS INSTRUCTION (8XY6 and 8XYE) ***************
				// On CHIP-48 and SUPER-CHIP, they no longer set 
				// VX to VY before the shift; Will make this configurable
				// later.
				case 0x6: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4; 
					
					uint8_t VY = chip8_state.v[y]; 

					chip8_state.v[x] = chip8_state.v[y]; 
					chip8_state.v[x] >>= 1; 
					chip8_state.v[0xF] = (VY) & 1; 
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
					uint8_t VX = chip8_state.v[x]; 
					uint8_t VY = chip8_state.v[y]; 
	
					chip8_state.v[x] = chip8_state.v[y] - chip8_state.v[x];
					
					if(VY < VX) {
						chip8_state.v[0xF] = 0; 
					} else {
						chip8_state.v[0xF] = 1;	
					}
				} break;
				case 0xE: {
					uint8_t x = (inst & 0x0F00) >> 8; 
					uint8_t y = (inst & 0x00F0) >> 4;
						

					uint8_t VY = chip8_state.v[y]; 

					chip8_state.v[x] = chip8_state.v[y]; 
					chip8_state.v[x] <<= 1;
					chip8_state.v[0xF] = (VY & 0x0080) >> 7; 
				} break; 
				default: {
					unimplemented_instruction(inst); 
				}
			}
		} break; 
		case 0x9: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t y = (inst & 0x00F0) >> 4;
			
			// Skip following instruction (increment pc by 2) if value of VX does not equal VY
			if(chip8_state.v[x] != chip8_state.v[y]) {
				chip8_state.pc += 2; 
			}
		} break; 
		case 0xA: {
			uint16_t NNN = (inst & 0x0FFF); 
			chip8_state.I = NNN; 
		} break; 
		// *************** AMBIGUOUS INSTRUCTION (BNNN) *************** 
		// CHIP-48 and SUPER-CHIP changed to BXNN, where it will jump to XNN + the value in v[x]. 
		case 0xB: {
			uint16_t NNN = (inst & 0x0FFF); 
			chip8_state.pc = NNN + chip8_state.v[0]; 
		} break; 
		case 0xC: {
			uint8_t x = (inst & 0xF00) >> 8; 
			uint8_t NN = (inst & 0x00FF);
			uint8_t random = (rand() % (UINT8_MAX + 1)); 
			
			chip8_state.v[x] = random & NN; 
		} break;  
		case 0xD: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			uint8_t y = (inst & 0x00F0) >> 4; 
			uint8_t N = (inst & 0x000F);
			
			int x_coord = chip8_state.v[x] % DISPLAY_WIDTH; 
			int y_coord = chip8_state.v[y] % DISPLAY_HEIGHT; 	
			
			uint16_t sprite_addr = chip8_state.I;
			
			bool swap = false; 

			for(int i = y_coord; i < y_coord + N && i < DISPLAY_HEIGHT; i++) {
				if(sprite_addr >= CHIP8_RAM) {
					printf("Error! Index register out of bounds!");
					error(); 
					break;  
				}
				uint8_t sprite_layer = chip8_state.memory[sprite_addr];
				for(int j = x_coord, k = 0; k < 8 && j < DISPLAY_WIDTH; j++, k++) {
					bool last_bit = display[i][j]; 
					display[i][j] ^= ((sprite_layer & (0x80 >> k)) >> (8 - (k + 1))); 
					
					// If set bit is unset, set VF register to 1. 
					// Otherwise, set VF to 0.
					if(last_bit && last_bit != display[i][j]) {
						swap = true; 
					}
				}
				sprite_addr++; 
			}
			
			if(swap) {
				chip8_state.v[0xF] = 1; 		
			} else {
				chip8_state.v[0xF] = 0;
			}
		} break; 
		case 0xE: {
			uint8_t x = (inst & 0x0F00) >> 8; 
			switch(inst & 0x00FF) {
				case 0x9E: {
					if(keyboard.is_pressed((chip8_state.v[x]) & 0x0F)) {
						chip8_state.pc += 2; 
					}
				} break; 
				case 0xA1: {
					if(!(keyboard.is_pressed((chip8_state.v[x]) & 0x0F))) {
						chip8_state.pc += 2; 
					}
				} break; 
			}	
		} break; 
		case 0xF: {
			uint8_t x = (inst & 0x0F00) >> 8; 	
			switch(inst & 0x00FF) {
				case 0x07: {
					chip8_state.v[x] = chip8_state.delay_timer; 	
				} break;
				case 0x0A: {
					if(keyboard.get_pad_release() != 0x10) {
						chip8_state.v[x] = keyboard.get_pad_release(); 
					} else {
						// Halt execution until key is given 
						chip8_state.pc -= 2; 
					}		
				} break;	
				case 0x15: {
					chip8_state.delay_timer = chip8_state.v[x]; 
				} break; 
				case 0x18: {
					chip8_state.sound_timer = chip8_state.v[x]; 
				} break; 
				case 0x1E: {
					// This instruction has a weird quirk on the Amiga, 
					// that causes the carry flag to be set. 
					// Will make this configurable later. 
					uint8_t I = chip8_state.I; 
					chip8_state.I += chip8_state.v[x]; 	
					
					if(I >= CHIP8_RAM - chip8_state.v[x]) {
						chip8_state.v[0xF] = 1; 	
					} else {
						chip8_state.v[0xF] = 0;
					}
				} break; 
				case 0x29: {
					uint8_t vx_char = chip8_state.v[x] & 0x0F; 
					// 0 -> address 80. 
					// 1 -> address 85. 
					// 2 -> address 90. 
					// Linear relationship, between vx_char and address.
					// y = 80 + 5 * vx_char. 
					chip8_state.I = 0x50 + (5 * vx_char); 
				} break; 
				case 0x33: {
					uint8_t val = chip8_state.v[x]; 
					uint16_t addr = chip8_state.I;
					if(addr < CHIP8_RAM) {
						chip8_state.memory[addr] = (val / 100); 
					} else {
						printf("Could not set BCD value! Address out of bounds!"); 
						error(); 
					}
					if(addr + 1 < CHIP8_RAM) {
						chip8_state.memory[addr + 1] = (val / 10) % 10;  
					} else {
						printf("Could not set BCD value! Address out of bounds!"); 
						error(); 
					}
					if(addr + 2 < CHIP8_RAM) {
						chip8_state.memory[addr + 2] = val % 10; 
					} else {
						printf("Could not set BCD value! Address out of bounds!"); 
						error(); 
					}
				} break; 
				case 0x55: {
					uint16_t addr = chip8_state.I; 
					for(uint8_t i = 0; i <= x; i++) {
						if(addr >= CHIP8_RAM) {
							printf("Could not load memory! address out of bounds!");
							error(); 
							break;
						}
						chip8_state.memory[addr] = chip8_state.v[i]; 
						addr++; 
					}
					chip8_state.I += x + 1;
					// *************** AMBIGUOUS INSTRUCTION *************** 
					// CHIP-48 and Super CHIP changed this so that I is not incremented to 
					// I + X + 1 after the operation. 
				} break; 
				case 0x65: {
					uint16_t addr = chip8_state.I; 
					for(uint8_t i = 0; i <= x; i++) {
						if(addr >= CHIP8_RAM) {
							printf("Could not store memory! address out of bounds!");
							error(); 
							break;
						}
						chip8_state.v[i] = chip8_state.memory[addr]; 
						addr++; 
					}
					chip8_state.I += x + 1; 
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
	for(uint64_t i = 0; i < CYCLES_PER_FRAME; i++) {
		if(!power) {
			continue;
		}
		uint16_t curr_inst = fetch_instruction(); 
		execute_instruction(curr_inst); 
	}
}

void CHIP8::decrement_timers() { // Seperate method, since these need to be decrement independant of the CPU speed
	if(chip8_state.delay_timer > 0) {
		chip8_state.delay_timer--; 
	}
	if(chip8_state.sound_timer > 0) {
		chip8_state.sound_timer--; 
	}
}

void CHIP8::update_keys() {
	keyboard.update_keys(); 
}

CHIP8::~CHIP8() {}
