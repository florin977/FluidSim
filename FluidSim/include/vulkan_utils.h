#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>

typedef struct {
    int graphicsFamily;
    int presentFamily;
} QueueFamilyIndices;

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount;
    VkSurfaceFormatKHR* formats;
    uint32_t presentModeCount;
    VkPresentModeKHR* presentModes;
} SwapChainSupportDetails;

extern VkInstance instance;
extern VkSurfaceKHR surface;
extern VkDevice device;
extern VkQueue graphicsQueue;
extern VkQueue presentQueue;
extern VkPhysicalDevice physicalDevice;

extern unsigned int logicalDeviceExtensionCount;
extern const char* requiredExtensions[];

extern const char* validationLayers[];

extern VkSwapchainKHR swapChain;
extern VkFormat swapChainImageFormat;
extern VkExtent2D swapChainExtent;

extern uint32_t imageCount;
extern VkImage* swapChainImages;
extern VkImageView* swapChainImageViews;


void baseSetupVulkan(SDL_Window *window);

VkPhysicalDevice selectGPU(VkInstance instance);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const uint32_t availableFormatsCount, const VkSurfaceFormatKHR* availableFormats);

VkPresentModeKHR chooseSwapPresentMode(const uint32_t availablePresentModesCount, const VkPresentModeKHR* availablePresentModes);

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR* capabilitites, SDL_Window* window);

void createSwapChain();

void createImageViews();

void initVulkan(SDL_Window* window);

void quitVulkan();

#endif
