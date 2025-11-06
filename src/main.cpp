#define SDL_MAIN_USE_CALLBACKS 1
#define FPS 60
#include <fstream>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "chip8.h"

struct app_context {
	SDL_Window* window; 
	SDL_Renderer* renderer;
	CHIP8* chip8; 
	uint32_t scale; 
}; 

void load_program(const std::string& file_name, app_context* ctx) {
	using namespace std; 
	
	ifstream is{file_name, ifstream::binary};
	if(is) {
		is.seekg(0, is.end);
		int length = is.tellg(); 
		is.seekg(0, is.beg); 
		char* codebuffer = new char[length]; 
		
		is.read(codebuffer, length); 
		ctx->chip8 = new CHIP8(codebuffer, length); 
		is.close(); 
		
		delete[] codebuffer; 
	} else {
		printf("Could not load program file!");
	}
}

SDL_AppResult app_fail() {
	SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError()); 
	return SDL_APP_FAILURE; 
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	if(!SDL_Init(SDL_INIT_VIDEO)) {
		return app_fail(); 
	}	
	
	uint32_t init_scale = 20;

	SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator", DISPLAY_WIDTH * init_scale, DISPLAY_HEIGHT * init_scale, 0); 
	if(!window) {
		return app_fail(); 
	}
	
	SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL); 
	if(!renderer) {
		return app_fail(); 
	}

	*appstate = new app_context {
		.window = window, 
		.renderer = renderer, 
		.scale = init_scale,
	}; 

	if(argc < 2) {
		SDL_Log("No program specified for load"); 
	} else {
		load_program(argv[1], (app_context*)(*appstate)); 		
	}

	return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event *event) {
	if(event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS; 	
	}
	return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	auto* app = (app_context*)appstate; 
	
	uint64_t now = SDL_GetTicksNS(); 
	
	SDL_FRect r; 
	r.w = r.h = app->scale; 

	SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); 	
	SDL_RenderClear(app->renderer);
	if(app->chip8 != nullptr) {	
		if(app->chip8->is_on()) {
			app->chip8->update_keystate(); 
			app->chip8->decrement_timers(); // Do this seprately; Needs to be 60Hz
			app->chip8->CPU_step();	
			// we need to draw the display now. 1 is for white pixel; 0 is for black
			for(int i = 0; i < DISPLAY_HEIGHT; i++) {
				for(int j = 0; j < DISPLAY_WIDTH; j++) {
					if(!(app->chip8->display_bit(i, j))) {
						continue; 	
					}
					r.x = j * app->scale; 
					r.y = i * app->scale; 
					SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
					SDL_RenderFillRect(app->renderer, &r);
				}
			}
		}
	}	
	SDL_RenderPresent(app->renderer); 
	
	uint64_t draw_time = SDL_GetTicksNS() - now; 	
	uint64_t ns_per_frame = (1000000000) / FPS;  

	if(draw_time < ns_per_frame) {
		SDL_DelayPrecise(ns_per_frame - draw_time); 
	}

	return SDL_APP_CONTINUE; 	
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	auto* app = (app_context*)appstate; 

	if(app->renderer != nullptr) {
		SDL_DestroyRenderer(app->renderer); 
	}
	
	if(app->window != nullptr) {
		SDL_DestroyWindow(app->window); 	
	}

	if(app->chip8 != nullptr) {
		delete app->chip8;
	}
}
