#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>

#include "QueueFamilyIndices.h"
#include "SwapchainSupportDetails.h"

class DeviceManager
{
private:

    DeviceManager() {}

    // Logical & physical device handles
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties properties;

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

public:

    // Return singleton instance
    static DeviceManager& instance();

    // Ensure singleton is never copied
    DeviceManager(DeviceManager const&)    = delete;
    void operator=(DeviceManager const&)   = delete;

    // Return logical & physical device handles
    VkDevice getDevice();
    VkPhysicalDevice getPhysicalDevice();
    VkPhysicalDeviceProperties getProperties();

    // Set up logical & physical device handles
    void pickPhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface);
    void createLogicalDevice(VkSurfaceKHR& surface, VkQueue& graphicsQueue, VkQueue& presentQueue, bool enableValidationLayers, const std::vector<const char*>& validationLayers);

    // Ensure a given device supports required queue families
    bool isDeviceSuitable(VkPhysicalDevice potentialDevice, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice potentialDevice);

    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice potentialDevice, VkSurfaceKHR surface);
};