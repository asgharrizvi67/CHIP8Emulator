#ifndef CHIP8_H 
#define CHIP8_H
#define CHIP8_RAM 4096 
#define FONT_ADDRESS 80
#define PROGRAM_ADDRESS 512
#define DISPLAY_WIDTH 64 
#define DISPLAY_HEIGHT 32 
#define CYCLES_PER_FRAME 1000 
#include "keyboard.h"
#include <stack>

struct cpu_state {
	uint8_t delay_timer; 
	uint8_t sound_timer; 
	uint16_t I; // Index register 
	uint16_t pc; // Program counter. Technically only needs 12 bits.  
	uint8_t v[16]; // v0 -> vf registers 
	uint8_t memory[CHIP8_RAM]; // RAM; All memory is editable 
	std::stack<uint16_t> st; // stack for functions and such; Calls functions at that address. 
}; 

class CHIP8 {
private: 
	bool power; 
	bool display[DISPLAY_HEIGHT][DISPLAY_WIDTH]; 
	cpu_state chip8_state;
	Keyboard keyboard; 
public: 
	CHIP8(char* codebuffer, int length); 
	uint16_t fetch_instruction(); 
	bool is_on();
	bool sound(); 
	bool display_bit(int i, int j);
	void execute_instruction(uint16_t inst);
	void CPU_step(); 
	void error();
	void decrement_timers(); 
	void update_keys(); 
	~CHIP8();
}; 

#endif
