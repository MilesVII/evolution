#include <SDL2/SDL.h>

#include "io.h"

SDL_Event event;

void io_init(){}

void io_getMouse(int* x, int* y){
	SDL_GetMouseState(x, y);
}

IOEvent io_getEvent(){
	SDL_PollEvent(&event);
	switch (event.type){
	case SDL_QUIT:
		return IO_QUIT;
	case SDL_MOUSEBUTTONDOWN:
		return IO_CLICK;
	default:
		return IO_NONE;
	}
}