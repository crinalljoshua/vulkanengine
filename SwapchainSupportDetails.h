#pragma once

#include <vector>

class SwapchainSupportDetails
{
public:

    VkSurfaceCapabilitiesKHR capabilities;

    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};