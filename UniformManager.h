#pragma once

#include <glm/glm.hpp>

#include "Utils.h"

class UniformManager
{
private:

    UniformManager() {}

    // Coherent uniform buffer and memory
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    // Dynamic uniform buffer and memory
    VkBuffer dynamicUniformBuffer;
    VkDeviceMemory dynamicMemory;

public:

    static UniformManager& instance();

    // Ensure singleton is never copied
    UniformManager(UniformManager const&)   = delete;
    void operator=(UniformManager const&)   = delete;

    struct DynamicUbo
    {
        glm::mat4 model;
        glm::mat4 norm;
    };

    struct StaticUbo
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    VkBuffer getCoherentUniformBuffer();
    VkDeviceMemory getCoherentMemory();

    VkBuffer getDynamicUniformBuffer();
    VkDeviceMemory getDynamicMemory();

    static DynamicUbo createDynamicUbo(glm::mat4 modelMatrix, glm::mat4 viewMatrix);

    // Create coherent/dynamic uniform buffers
    void createUniformBuffer();
    void createDynamicUniformBuffer(size_t &dynamicAlignment, size_t objectCount);

    void cleanupUniformBuffers();
};