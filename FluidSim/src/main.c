#include "../include/sdl_init.h"
#include "../include/vulkan_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void drawFrame()
{
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX); // Device, number of fences, array of fences, wait on all fences or not, timeout (in nanoseconds)
    vkResetFences(device, 1, &inFlightFence);                        // Reset the fences manually to continue execution
    uint32_t imageIndex = 0;

    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, imageIndex);

    // Submiting the command buffer

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to submit draw command buffer!\n");
        exit(EXIT_FAILURE);
    }

    // Draw image to the swapchain

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL; // Optional - used to check results between multiple swapchains

    vkQueuePresentKHR(presentQueue, &presentInfo);
}

int main(int argc, char *argv[])
{
    setupWindow(&window);

    initVulkan(window);

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

        // Vulkan rendering here

        drawFrame();
    }

    vkDeviceWaitIdle(device);

    quitVulkan();
    quitSDL(&window);

    return 0;
}