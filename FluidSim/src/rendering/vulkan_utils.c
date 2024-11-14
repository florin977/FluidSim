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

const int enableValidationLayers = 1; // Turn off for release

const char *validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"};

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

void baseSetupVulkan(SDL_Window *window)
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
    if (!SDL_Vulkan_GetInstanceExtensions(&extensionCount))
    {
        printf("Failed to get the number of Vulkan extensions required: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    if (extensionCount == 0)
    {
        printf("Failed to get the number of Vulkan extensions required: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

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

        // Query device features
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // Check for suitable device
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader)
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
        }
    }

    free(queueFamilies);

    if (indices.graphicsFamily == -1)
    {
        fprintf(stderr, "Error: Required queue families not found.\n");
        exit(EXIT_FAILURE);
    }

    printf("Indices: %d, %d\n", indices.graphicsFamily, indices.presentFamily);

    return indices;
}

void createLogicalDevice()
{

    // Number of queues for a queue family. Only one wirh graphics for now
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
    queueCreateInfo.queueCount = 1;

    // Priority of the queue
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    // Features of the device
    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = 0;

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

    printf("Logical device created succesfully\n");
}

void initVulkan(SDL_Window *window)
{
    baseSetupVulkan(window);
    physicalDevice = selectGPU(instance);
    indices = findQueueFamilies(physicalDevice);
    createLogicalDevice();
}

void quitVulkan()
{
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, NULL);
        device = VK_NULL_HANDLE;
    }
    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, NULL);
        surface = VK_NULL_HANDLE;
    }
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, NULL);
        instance = VK_NULL_HANDLE;
    }
}
