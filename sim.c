#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <stdio.h>
#include <stdlib.h>

void setupWindow(SDL_Window **window)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Couldn't initialize SDL: %s \n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    *window = SDL_CreateWindow("SDL Vulkan Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_VULKAN);

    if (!*window)
    {
        printf("Failed to open Vulkan window: %s\n", SDL_GetError());
        SDL_Quit();

        exit(EXIT_FAILURE);
    }
}

void setupVulkan()
{

}

void handleEvents(SDL_Event *event, int *running)
{
    
}

void quitSDL(SDL_Window **window)
{
    SDL_DestroyWindow(*window);
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    SDL_Window *window = NULL;
    
    setupWindow(&window);

    setupVulkan();

    SDL_Event event;

    int running = 1;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }
        }

        //Vulkan rendering here
    }    

    quitSDL(&window);

    return 0;
}