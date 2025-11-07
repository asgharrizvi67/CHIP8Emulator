#include <map>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <cstring>
#include "keyboard.h"

/*
CHIP8 Keypad Layout: 
	1 2 3 C
	4 5 6 D
	7 8 9 E
	A 0 B F

Default Key Mapping: 
	1 2 3 4
	Q W E R 
	A S D F
	Z X C V 
*/

Keyboard::Keyboard() {
	curr_keys = SDL_GetKeyboardState(NULL);  
	// Set default configuration
	padtokey[0x0] = SDL_SCANCODE_X; 
	padtokey[0x1] = SDL_SCANCODE_1; 
	padtokey[0x2] = SDL_SCANCODE_2; 
	padtokey[0x3] = SDL_SCANCODE_3; 
	padtokey[0x4] = SDL_SCANCODE_Q; 
	padtokey[0x5] = SDL_SCANCODE_W; 
	padtokey[0x6] = SDL_SCANCODE_E; 
	padtokey[0x7] = SDL_SCANCODE_A; 
	padtokey[0x8] = SDL_SCANCODE_S; 
	padtokey[0x9] = SDL_SCANCODE_D; 
	padtokey[0xA] = SDL_SCANCODE_Z; 
	padtokey[0xB] = SDL_SCANCODE_C; 
	padtokey[0xC] = SDL_SCANCODE_4; 
	padtokey[0xD] = SDL_SCANCODE_R; 
	padtokey[0xE] = SDL_SCANCODE_F; 
	padtokey[0xF] = SDL_SCANCODE_V; 
}

void Keyboard::update_keys() {
	std::memcpy(old_keys, curr_keys, sizeof(old_keys)); 	
}

bool Keyboard::is_pressed(uint8_t pad_val) {
	return curr_keys[padtokey[pad_val]]; 
}

uint8_t Keyboard::get_pad_release() {
	for(uint8_t i = 0; i < 16; i++) {
		if(old_keys[padtokey[i]] && !curr_keys[padtokey[i]]) {
			return i; 
		}
	}
	return 0x10; 
}

Keyboard::~Keyboard(){} 
