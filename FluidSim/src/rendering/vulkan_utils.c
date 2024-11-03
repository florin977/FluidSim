#include "vulkan_utils.h"
#include <stdlib.h>
#include <stdio.h>

VkInstance instance = VK_NULL_HANDLE;

void setupVulkan(SDL_Window *window) // Change return type to int
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fluid Simulation";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Custom Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    unsigned int extensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL))
    {
        printf("Failed to get the number of Vulkan extensions required: %s\n", SDL_GetError());
        exit(EXIT_FAILURE); // Indicate failure
    }

    const char **extensions = malloc(sizeof(const char *) * extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions))
    {
        printf("Failed to get Vulkan extensions: %s\n", SDL_GetError());
        free(extensions);
        exit(EXIT_FAILURE); // Indicate failure
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS)
    {
        printf("Failed to create Vulkan instance.\n");
        free(extensions);
        exit(EXIT_FAILURE); // Indicate failure
    }

    free(extensions);

    printf("Vulkan instance created successfully.\n"); // Indicate success
}

VkPhysicalDevice selectGPU(VkInstance instance) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    
    if (deviceCount == 0) {
        fprintf(stderr, "Failed to find GPUs with Vulkan support!\n");
        return VK_NULL_HANDLE; // Indicate failure
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDevice device = devices[i];

        // Query device properties
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Check for suitable device
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            printf("Selected GPU: %s\n", deviceProperties.deviceName);
            selectedDevice = device;
            break; // Stop searching if we found a discrete GPU
        }
    }

    free(devices);

    if (selectedDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to find a suitable GPU!\n");
    }

    return selectedDevice;
}

void quitVulkan() {
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, NULL);
        instance = VK_NULL_HANDLE; // Reset the instance
    }
}
