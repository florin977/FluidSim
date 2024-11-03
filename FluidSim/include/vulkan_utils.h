#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#include <SDL.h>
#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>

extern VkInstance instance;

void setupVulkan(SDL_Window *window);

VkPhysicalDevice selectGPU(VkInstance instance);

void quitVulkan();

#endif
