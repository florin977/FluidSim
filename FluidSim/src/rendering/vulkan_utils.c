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

uint32_t imageCount = 0;
VkImage *swapChainImages;
VkImageView *swapChainImageViews;

VkDynamicState dynamicStates[2] = { // Modify number of elements ?
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR};

VkRenderPass renderPass = VK_NULL_HANDLE;
VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
VkPipeline graphicsPipeline = VK_NULL_HANDLE;

VkFramebuffer *swapChainFramebuffers = NULL;
VkCommandPool commandPool = VK_NULL_HANDLE;
VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;

const int enableValidationLayers = 1; // Turn off for release

unsigned int logicalDeviceExtensionCount = 1;

const char *requiredExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

    const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

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

void createSurface(SDL_Window *window)
{
    if (!SDL_Vulkan_CreateSurface(window, instance, NULL, &surface))
    {
        fprintf(stderr, "Failed to create surface: %s!\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    printf("Surface created succesfully\n");
}

int isDeviceCompatible(VkPhysicalDevice device)
{
    // Reset indices
    indices.graphicsFamily = -1;
    indices.presentFamily = -1;

    // Find queue families
    indices = findQueueFamilies(device);

    // Check if both graphics and present queue families are valid
    if (indices.graphicsFamily == -1 || indices.presentFamily == -1)
    {
        return 0; // Not compatible
    }

    // Query device properties
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    printf("Testing GPU: %s\n", deviceProperties.deviceName);

    if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        return 0; // Device is not a disrete GPU
    }

    // Query device features
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Ensure the device supports necessary features
    if (!deviceFeatures.geometryShader)
    {
        return 0; // Geometry shader support is mandatory
    }

    // Check for required extensions
    unsigned int extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties *availableExtensions = malloc(extensionCount * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

    int extensionFound = 0;

    for (unsigned long long int i = 0; i < sizeof(requiredExtensions) / sizeof(requiredExtensions[0]); i++)
    {
        extensionFound = 0;
        for (unsigned int j = 0; j < extensionCount; j++)
        {
            if (strcmp(requiredExtensions[i], availableExtensions[j].extensionName) == 0)
            {
                extensionFound = 1;
                break;
            }
        }
        if (!extensionFound)
        {
            break;
        }
    }

    free(availableExtensions);

    // If any required extension is missing, the device is not compatible
    if (!extensionFound)
    {
        return 0;
    }

    // Check if it works with the SwapChain

    uint8_t swapChainAdequate = 0;

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);

    swapChainAdequate = (swapChainSupport.formats != NULL && swapChainSupport.presentModes != NULL);

    if (!swapChainAdequate)
    {
        return 0; // Not Compatible
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

    // Extensions needed for the swapchain
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

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    // Get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Get formatCount and formats

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

    // Get presentModeCount and presentModes

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

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const uint32_t availableFormatsCount, const VkSurfaceFormatKHR *availableFormats)
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

VkPresentModeKHR chooseSwapPresentMode(const uint32_t availablePresentModesCount, const VkPresentModeKHR *availablePresentModes)
{
    for (int i = 0; i < availablePresentModesCount; i++)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities, SDL_Window *window)
{
    // Use the surface's fixed size if available
    if ((capabilities->currentExtent.width != UINT32_MAX))
    {
        printf("Surface current extent: %dx%d\n", capabilities->currentExtent.width, capabilities->currentExtent.height);
        return capabilities->currentExtent;
    }
    else
    {
        // Query SDL window size in pixels
        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);

        printf("Getting Surface Extent: Width: %d\nHeight: %d\n", width, height);

        // Create an extent based on the window size

        VkExtent2D actualExtent = {
            .width = (uint32_t)width,
            .height = (uint32_t)height};

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

void createSwapChain(SDL_Window *window)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formatCount, swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModeCount, swapChainSupport.presentModes);
    swapChainExtent = chooseSwapExtent(&swapChainSupport.capabilities, window);

    // To ensure the image views use the same format
    swapChainImageFormat = surfaceFormat.format;

    imageCount = swapChainSupport.capabilities.minImageCount + 1;

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
    createInfo.imageExtent = swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Handle different/same queue families
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
        createInfo.queueFamilyIndexCount = 0;  // Optional
        createInfo.pQueueFamilyIndices = NULL; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // In case another window blocks the image, do NOT render the overshadowed pixels

    createInfo.oldSwapchain = VK_NULL_HANDLE; // In case the swap cahins becomes invalid/unoptimized

    free(swapChainSupport.formats);
    free(swapChainSupport.presentModes);
    if (vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create the swap cahin!\n");
        exit(EXIT_FAILURE);
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL);

    swapChainImages = malloc(imageCount * sizeof(VkImage));

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages);

    printf("Swap Chain created successfully\n");
}

void createImageViews()
{
    swapChainImageViews = malloc(imageCount * sizeof(VkImageView));

    for (int i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create image views!\n");
            exit(EXIT_FAILURE);
        }

        printf("Image Views created successfully\n");
    }
}

static char *readFile(const char *filename, size_t *outSize)
{
    // Open the file in binary mode
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("failed to open file");
        return NULL;
    }

    // Seek to the end of the file to determine the size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file); // Go back to the beginning of the file

    // Allocate memory to store the file's contents
    char *buffer = (char *)malloc(fileSize);
    if (!buffer)
    {
        perror("failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read the contents of the file into the buffer
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead != fileSize)
    {
        fprintf(stderr, "failed to read the entire file");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Close the file
    fclose(file);

    // Set the size of the file (for later use)
    *outSize = fileSize;

    // Return the buffer containing the file's contents
    return buffer;
}

VkShaderModule createShaderModule(VkDevice device, const char *shaderCode, size_t codeSize)
{
    // Create the VkShaderModuleCreateInfo structure
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = (const uint32_t *)shaderCode; // Cast the byte array to uint32_t*

    // Create the shader module
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create shader module!\n");
        exit(EXIT_FAILURE);
    }

    return shaderModule; // Return the created shader module
}

void createRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Subpasses (one for now, maybe more for postprocessing in the future)
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive

    // Dependencies to make sure we do not write while the swap chain is reading from the image

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create render pass!\n");
        exit(EXIT_FAILURE);
    }
}

void createGraphicsPipeline()
{
    size_t vertShaderSize, fragShaderSize;

    // Load the shader files
    char *vertShaderCode = readFile("../shaders/vert.spv", &vertShaderSize);
    char *fragShaderCode = readFile("../shaders/frag.spv", &fragShaderSize);

    // Check if shaders were loaded successfully
    if (vertShaderCode == NULL || fragShaderCode == NULL)
    {
        fprintf(stderr, "failed to load shader files!\n");
        free(vertShaderCode);
        free(fragShaderCode);
        return;
    }

    // Print the size of the loaded shaders to verify the file sizes
    printf("Vertex shader size: %zu bytes\n", vertShaderSize);
    printf("Fragment shader size: %zu bytes\n", fragShaderSize);

    // Create the shader modules
    VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode, vertShaderSize);
    VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode, fragShaderSize);

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE)
    {
        fprintf(stderr, "failed to create shader modules!\n");
        free(vertShaderCode);
        free(fragShaderCode);
        return;
    }

    // Free the shader code memory (no longer needed after creating shader modules)
    free(vertShaderCode);
    free(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Dynamic State
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2; // Sizeof(dynamicStates) at line 20
    dynamicState.pDynamicStates = dynamicStates;

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = NULL; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = NULL; // Optional

    // How vertex data is used/parsed
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Modify in _LINE_STRIP to use last 2 vertices as the start of the third one
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissors
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swapChainExtent;

    // Regarding dynamicStates
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // TO DO: Read more about culling, this can be used to enable wireframe mode
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f;          // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;          // Optional
    multisampling.pSampleMask = NULL;               // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // TO DO: Read more about color blending/ play with these arguments a little

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;   // IMPORTANT! Might use the wrong method of color blending
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;         // No descriptor sets
    pipelineLayoutInfo.pSetLayouts = NULL;         // Pointer to descriptor set layouts (none here)
    pipelineLayoutInfo.pushConstantRangeCount = 0; // No push constants
    pipelineLayoutInfo.pPushConstantRanges = NULL; // Pointer to push constant ranges (none here)

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create pipeline layout!\n");
        exit(EXIT_FAILURE);
    }

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = NULL; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    // This can be used to create a "new" pipeline from an existing one, as it is faster than starting from zero
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create graphics pipeline!\n");
        exit(EXIT_FAILURE);
    }

    // Cleanup: Destroy the shader modules
    vkDestroyShaderModule(device, fragShaderModule, NULL);
    vkDestroyShaderModule(device, vertShaderModule, NULL);

    printf("Graphics pipeline created successfully\n");
}

void createFrameBuffers()
{
    swapChainFramebuffers = malloc(imageCount * sizeof(VkFramebuffer));

    for (int i = 0; i < imageCount; i++)
    {
        VkImageView attachments[] = {
            swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create framebuffers\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("Framebuffers created successfully\n");
}

void createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily;

    if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create command pool!\n");
        exit(EXIT_FAILURE);
    }

    printf("Command pool created successfully\n");
}

void createCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1; // Number of command buffers

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to allocate command buffers!\n");
        exit(EXIT_FAILURE);
    }

    printf("Command buffer created successfully\n");
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;               // Optional
    beginInfo.pInheritanceInfo = NULL; // Optional - used for secondary command buffers

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to begin recording command buffer!\n");
        exit(EXIT_FAILURE);
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // Clear the screen using black as a color
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // Viewport and scissors are dynamic, so we specify them here
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapChainExtent.width;   // Cast to float ?
    viewport.height = swapChainExtent.height; // Cast to float ?
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swapChainExtent;

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Draw command, second argument is the number of instances and the 3rd and 4th are the offsets for the instances
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to record command buffer!\n");
        exit(EXIT_FAILURE);
    }
}

void createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Fence is signaled so we do not block the execution indefinitely

    if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, NULL, &inFlightFence) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create semaphores!\n");
        exit(EXIT_FAILURE);
    }
}

void initVulkan(SDL_Window *window)
{
    baseSetupVulkan(window);
    createSurface(window);
    physicalDevice = selectGPU(instance);
    createLogicalDevice();
    createSwapChain(window);
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

void quitVulkan()
{
    if (device != VK_NULL_HANDLE)
    {
        if (commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, commandPool, NULL); // Also destroyes commandbuffers
            printf("Destroyed command pool\n");
        }

        if (graphicsPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, graphicsPipeline, NULL);
            printf("Destroyed graphics pipeline\n");
        }

        if (pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, pipelineLayout, NULL);
            printf("Destroyed pipeline layout\n");
        }

        if (swapChainFramebuffers != NULL)
        {
            for (int i = 0; i < imageCount; i++)
            {
                vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
            }
            printf("Destroyed framebuffers\n");
        }

        free(swapChainFramebuffers);
        swapChainFramebuffers = NULL;

        if (renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device, renderPass, NULL);
            printf("Destroyed renderPass\n");
        }

        vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
        vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
        vkDestroyFence(device, inFlightFence, NULL);

        if (swapChain != VK_NULL_HANDLE)
        {
            if (swapChainImageViews != NULL)
            {
                for (int i = 0; i < imageCount; i++)
                {
                    vkDestroyImageView(device, swapChainImageViews[i], NULL);
                }
            }

            free(swapChainImageViews);
            swapChainImageViews = NULL;

            free(swapChainImages);
            swapChainImages = NULL;

            vkDestroySwapchainKHR(device, swapChain, NULL);
            printf("Destroyed swap chain\n");

            swapChain = VK_NULL_HANDLE;
        }

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
