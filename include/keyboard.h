#ifndef KEYBOARD_H
#define KEYBOARD_H 
#define SDL_KEY_LENGTH 512
#include <SDL3/SDL.h>
#include <map>


class Keyboard {
private: 
	SDL_Scancode padtokey[16];  
	bool old_keys[SDL_KEY_LENGTH]; 
	const bool* curr_keys; 
public: 
	Keyboard();
	void update_keys(); 
	bool is_pressed(uint8_t pad_val);
	uint8_t get_pad_release(); 
	~Keyboard(); 
}; 

#endif
