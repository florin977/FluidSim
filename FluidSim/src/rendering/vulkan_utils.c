#include "vulkan_utils.h"
#include <stdlib.h>
#include <stdio.h>

VkInstance instance = VK_NULL_HANDLE;
VkSurfaceKHR surface = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
VkQueue graphicsQueue = VK_NULL_HANDLE;
VkQueue presentQueue = VK_NULL_HANDLE;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
QueueFamilyIndices indices;

VkSwapchainKHR swapChain = VK_NULL_HANDLE;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

VkImage* swapChainImages;

const int enableValidationLayers = 1; // Turn off for release

unsigned int logicalDeviceExtensionCount = 1;

const char *requiredExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const char *validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

// Function to check if all requested validation layers are available
int checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (size_t i = 0; i < sizeof(validationLayers) / sizeof(validationLayers[0]); i++)
    {
        int layerFound = 0;

        for (uint32_t j = 0; j < layerCount; j++)
        {
            if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0)
            {
                layerFound = 1;
                break;
            }
        }

        if (!layerFound)
        {
            return 0;
        }
    }

    return 1;
}

void baseSetupVulkan(SDL_Window* window)
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        fprintf(stderr, "Validation layers requested, but not available.\n");
        exit(EXIT_FAILURE);
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fluid Simulation";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Custom Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    unsigned int extensionCount = 0;

    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    if (extensionCount == 0 || !extensions)
    {
        printf("Failed to get required Vulkan extensions: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
        createInfo.ppEnabledLayerNames = validationLayers;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS)
    {
        printf("Failed to create Vulkan instance.\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Vulkan instance created successfully.\n");
}

void createSurface(SDL_Window* window)
{
    if (!SDL_Vulkan_CreateSurface(window, instance, NULL, &surface))
    {

        fprintf(stderr, "Failed to create surface!\n");
        exit(EXIT_FAILURE);
    }

    printf("Surface created succesfully\n");
}

int isDeviceCompatible(VkPhysicalDevice device) {
    //Reset indices
    indices.graphicsFamily = -1;
    indices.presentFamily = -1;

    // Find queue families
    indices = findQueueFamilies(device);

    // Check if both graphics and present queue families are valid
    if (indices.graphicsFamily == -1 || indices.presentFamily == -1) {
        return 0; // Not compatible
    }

    // Query device properties
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    printf("Testing GPU: %s\n", deviceProperties.deviceName);

    if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        return 0; //Device is not a disrete GPU
    }

    // Query device features
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Ensure the device supports necessary features
    if (!deviceFeatures.geometryShader) {
        return 0; // Geometry shader support is mandatory
    }

    // Check for required extensions
    unsigned int extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties *availableExtensions = malloc(extensionCount * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

    int extensionFound = 0;

    for (unsigned long long int i = 0; i < sizeof(requiredExtensions) / sizeof(requiredExtensions[0]); i++) {
        extensionFound = 0;
        for (unsigned int j = 0; j < extensionCount; j++) {
            if (strcmp(requiredExtensions[i], availableExtensions[j].extensionName) == 0) {
                extensionFound = 1;
                break;
            }
        }
        if (!extensionFound) {
            break;
        }
    }

    free(availableExtensions);

    // If any required extension is missing, the device is not compatible
    if (!extensionFound) {
        return 0;
    }

    //Check if it works with the SwapChain

    uint8_t swapChainAdequate = 0;

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);

    swapChainAdequate = (swapChainSupport.formats != NULL && swapChainSupport.presentModes != NULL);

    if (!swapChainAdequate)
    {
        return 0; //Not Compatible
    }

    return 1; // Compatible
}

VkPhysicalDevice selectGPU(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    if (deviceCount == 0)
    {
        fprintf(stderr, "Failed to find GPUs with Vulkan support!\n");
        return VK_NULL_HANDLE;
    }

    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDevice device = devices[i];

        // Query device properties
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Check for suitable device
        if (isDeviceCompatible(device))
        {
            printf("Selected GPU: %s\n", deviceProperties.deviceName);
            selectedDevice = device;
            break; // Stop searching if we found a discrete GPU that supports a geometry shader
        }
    }

    free(devices);

    if (selectedDevice == VK_NULL_HANDLE)
    {
        fprintf(stderr, "Failed to find a suitable GPU!\n");
    }

    return selectedDevice;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = {-1, -1}; // Explicitly set to invalid values

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (int i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport)
            {
                indices.presentFamily = i;
            }
        }
    }

    free(queueFamilies);

    printf("Indices: %d, %d\n", indices.graphicsFamily, indices.presentFamily);

    return indices;
}

void createLogicalDevice()
{

    int uniqueQueueFamilies[2];
    int uniqueCount = 0;

    if (indices.graphicsFamily != indices.presentFamily)
    {
        uniqueQueueFamilies[0] = indices.graphicsFamily;
        uniqueQueueFamilies[1] = indices.presentFamily;
        uniqueCount = 2;
    }
    else 
    {
        uniqueQueueFamilies[0] = indices.graphicsFamily;
        uniqueCount = 1;
    }


    // Priority of the queue
    float queuePriority = 1.0f;
    
    // Create queue info for every queue we have
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
    
    for (int i = 0; i < uniqueCount; i++)
    {
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
    }

    // Features of the device
    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = uniqueCount;
    createInfo.pEnabledFeatures = &deviceFeatures;

    //Extensions needed for the swapchain
    createInfo.enabledExtensionCount = logicalDeviceExtensionCount;
    createInfo.ppEnabledExtensionNames = requiredExtensions;

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
        createInfo.ppEnabledLayerNames = validationLayers;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create logical device!\n");
        exit(EXIT_FAILURE);
    }

    // Get graphics queue handles
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);

    // Get present queue handles
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);

    printf("Logical device created succesfully\n");
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    //Get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    //Get formatCount and formats

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, NULL);

    if (details.formatCount != 0)
    {
        details.formats = malloc(details.formatCount * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, details.formats);
    }
    else
    {
        details.formats = NULL;
    }

    //Get presentModeCount and presentModes

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, NULL);
    
    if (details.presentModeCount != 0) 
    {
        details.presentModes = malloc(details.presentModeCount * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, details.presentModes);
    }
    else 
    {
        details.presentModes = NULL;
    }


    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const uint32_t availableFormatsCount, const VkSurfaceFormatKHR* availableFormats)
{
    for (int i = 0; i < availableFormatsCount; i++)
    {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormats[i];
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const uint32_t availablePresentModesCount, const VkPresentModeKHR* availablePresentModes) {
    for (int i = 0; i < availablePresentModesCount; i++)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window)
{
    // Use the surface's fixed size if available
    if (capabilities->currentExtent.width != UINT32_MAX)
    {
        return capabilities->currentExtent;
    }
    else 
    {
        // Query SDL window size in pixels
        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);

        // Create an extent based on the window size

         VkExtent2D actualExtent = {
            .width = (uint32_t)width,
            .height = (uint32_t)height
        };

        // Clamp the extent to the allowed range
        actualExtent.width = (actualExtent.width < capabilities->minImageExtent.width) 
                             ? capabilities->minImageExtent.width 
                             : (actualExtent.width > capabilities->maxImageExtent.width) 
                               ? capabilities->maxImageExtent.width 
                               : actualExtent.width;

        actualExtent.height = (actualExtent.height < capabilities->minImageExtent.height) 
                              ? capabilities->minImageExtent.height 
                              : (actualExtent.height > capabilities->maxImageExtent.height) 
                                ? capabilities->maxImageExtent.height 
                                : actualExtent.height;

        return actualExtent;
    }
}

void createSwapChain(SDL_Window* window) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formatCount, swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModeCount, swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(&swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    //Handle different/same queue families
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) 
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } 
    else 
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = NULL; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; //In case another window blocks the image, do NOT render the overshadowed pixels

    createInfo.oldSwapchain = VK_NULL_HANDLE; //In case the swap cahins becomes invalid/unoptimized

    free(swapChainSupport.formats);
    free(swapChainSupport.presentModes);
    if (vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain) != VK_SUCCESS) 
    {
        fprintf(stderr, "Failed to create the swap cahin!\n");
        exit(EXIT_FAILURE);
    }


    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL);

    swapChainImages = malloc(imageCount * sizeof(VkImage));
    //Save imageCount ?
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages);

    printf("Swap Chain created successfully\n");
}


void initVulkan(SDL_Window* window)
{
    baseSetupVulkan(window);
    createSurface(window);
    physicalDevice = selectGPU(instance);
    createLogicalDevice();
    createSwapChain(window);
}

void quitVulkan()
{
    if (device != VK_NULL_HANDLE && swapChain != VK_NULL_HANDLE)
    {
        free(swapChainImages);
        swapChainImages = NULL;
        vkDestroySwapchainKHR(device, swapChain, NULL);
        printf("Destroyed swap chain\n");
        swapChain = VK_NULL_HANDLE;
    }
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, NULL);
        printf("Destroyed device\n");
        device = VK_NULL_HANDLE;
    }
    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, NULL);
        printf("Destroyed surface\n");
        surface = VK_NULL_HANDLE;
    }
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, NULL);
        printf("Destroyed instance\n");
        instance = VK_NULL_HANDLE;
    }
}
