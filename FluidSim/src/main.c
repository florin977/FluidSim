#include "../include/sdl_init.h"
#include "../include/vulkan_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char* argv[])
{
    setupWindow(&window);

    initVulkan(window);

    //printf("Using GPU: %p\n", (void*)physicalDevice);
    
    SDL_Event event;

    int running = 1;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = 0;
            }
        }

        //Vulkan rendering here
    }    

    quitSDL(&window);

    return 0;
}