#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <vector>

#include "DeviceManager.h"
#include "SwapchainSupportDetails.h"

class SwapchainManager
{
private:

    SwapchainManager() {}

    // Vulkan swapchain members
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    std::vector<VkImage> swapchainImages;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    std::vector<VkImageView> swapchainImageViews;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

public:

    // Return singleton instance
    static SwapchainManager& instance();

    // Ensure singleton is never copied
    SwapchainManager(SwapchainManager const&)   = delete;
    void operator=(SwapchainManager const&)     = delete;

    // Expose class members
    VkSwapchainKHR getSwapchain();
    VkFormat getImageFormat();
    VkExtent2D getExtent();

    // Return member framebuffer & image view vectors
    std::vector<VkFramebuffer> getFramebuffers();
    std::vector<VkImageView> getImageViews();

    int getFramebufferSize();

    void createSwapchain(VkSurfaceKHR surface, GLFWwindow* window);
    void createImageViews();
    void createFramebuffers(VkImageView depthImage, VkRenderPass renderPass);
};