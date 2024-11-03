#include "../include/sdl_init.h"
#include <stdlib.h>
#include <stdio.h>

SDL_Window *window = NULL;

void setupWindow(SDL_Window **window) // Change void to int
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Couldn't initialize SDL: %s \n", SDL_GetError());
        exit(EXIT_FAILURE); // Indicate failure
    }

    *window = SDL_CreateWindow("SDL Vulkan Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_VULKAN);

    if (!*window)
    {
        printf("Failed to open Vulkan window: %s\n", SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE); // Indicate failure
    }

    printf("Window created successfully\n");
}

void quitSDL(SDL_Window **window)
{
    SDL_DestroyWindow(*window);
    SDL_Quit();
}
