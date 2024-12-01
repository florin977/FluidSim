#ifndef SDL_INIT_H
#define SDL_INIT_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>

extern SDL_Window* window;

void setupWindow(SDL_Window** window);

void quitSDL(SDL_Window** window);

#endif
