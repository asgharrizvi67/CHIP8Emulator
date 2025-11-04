#ifndef CHIP8_H 
#define CHIP8_H
#define CHIP8_RAM 4000 
#define FONT_ADDRESS 80
#define PROGRAM_ADDRESS 512
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define INSTRUCTIONS_PER_SECOND 1000

struct cpu_state; 

class CHIP8 {
private: 
	bool power; 
	bool display[DISPLAY_HEIGHT][DISPLAY_WIDTH]; 
	uint64_t last_cpu_time;
	cpu_state* chip8_state;
public: 
	CHIP8(char* codebuffer, int length); 
	uint16_t fetch_instruction(); 
	bool is_on();
	bool display_bit(int i, int j);
	void execute_instruction(uint16_t inst);
	void CPU_step(); 
	void error();
	~CHIP8();
}; 

#endif
