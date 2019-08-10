#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "utils.h"

const bool HARDWARE_RENDER = false;

SDL_Window* window;
SDL_Renderer* renderer;

void drawing_init(char* title, int w, int h){
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0){
		printf("SDL Init error: %s\n", SDL_GetError());
		SDL_Delay(7000);
		//return 0;
	}
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
	                          w, h, HARDWARE_RENDER ? SDL_WINDOW_OPENGL : 0);
	if (HARDWARE_RENDER){
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		if (renderer == NULL)
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	} else {
		SDL_Surface* surface = SDL_GetWindowSurface(window);
		renderer = SDL_CreateSoftwareRenderer(surface);
	}
	if (renderer == NULL){
		printf("Unable to initialize renderer: %s\n", SDL_GetError());
		SDL_Delay(7000);
	}
}

void drawing_setColor(uint32_t color){
	SDL_SetRenderDrawColor(renderer, decodeColorR(color), decodeColorG(color), decodeColorB(color), 0xFF);
}

void drawing_clear(uint32_t color){
	drawing_setColor(color);
	SDL_RenderClear(renderer);
}

void drawing_pixel(int x, int y){
	SDL_RenderDrawPoint(renderer, x, y);
}

void drawing_render(){
	if (HARDWARE_RENDER)
		SDL_RenderPresent(renderer);
	else
		SDL_UpdateWindowSurface(window);
}

void drawing_terminate(){
	SDL_DestroyWindow(window);
	SDL_Quit();
}
