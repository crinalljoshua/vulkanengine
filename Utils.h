#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include "DeviceManager.h"

class Utils
{
public:

    static void createBuffer(VkDeviceSize size, 
                             VkBufferUsageFlags usage, 
                             VkMemoryPropertyFlags properties, 
                             VkBuffer& buffer, VkDeviceMemory& 
                             bufferMemory);
                             
    static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};