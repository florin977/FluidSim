#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>

typedef struct {
    int graphicsFamily;
    int presentFamily;
} QueueFamilyIndices;

extern VkInstance instance;
extern VkSurfaceKHR surface;
extern VkDevice device;
extern VkQueue graphicsQueue;
extern VkQueue presentQueue;
extern VkPhysicalDevice physicalDevice;

extern const char* validationLayers[];

void baseSetupVulkan(SDL_Window *window);

VkPhysicalDevice selectGPU(VkInstance instance);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

void initVulkan(SDL_Window* window);

void quitVulkan();

#endif
