#include "../include/sdl_init.h"
#include <stdlib.h>
#include <stdio.h>

SDL_Window *window = NULL;

void setupWindow(SDL_Window** window)
{
    if (SDL_Init(SDL_INIT_VIDEO) == 0)
    {
        printf("Couldn't initialize SDL: %s \n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    *window = SDL_CreateWindow("SDL Vulkan Window", 1280, 720, SDL_WINDOW_VULKAN);

    if (!*window)
    {
        printf("Failed to open Vulkan window: %s\n", SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    printf("Window created successfully\n");
}

void quitSDL(SDL_Window** window)
{
    SDL_DestroyWindow(*window);
    SDL_Quit();
    printf("Destroyed window\n");
}
