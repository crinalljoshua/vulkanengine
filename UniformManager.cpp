#include "UniformManager.h"

UniformManager& UniformManager::instance()
{
    static UniformManager instance;
    
    return instance;
}

VkBuffer UniformManager::getCoherentUniformBuffer()
{
    return uniformBuffer;
}

VkDeviceMemory UniformManager::getCoherentMemory()
{
    return uniformBufferMemory;
}

VkBuffer UniformManager::getDynamicUniformBuffer()
{
    return dynamicUniformBuffer;
}

VkDeviceMemory UniformManager::getDynamicMemory()
{
    return dynamicMemory;
}

UniformManager::DynamicUbo UniformManager::createDynamicUbo(glm::mat4 modelMatrix, glm::mat4 viewMatrix)
{
    DynamicUbo ubo = {};

    // Set model and normal matrices
    ubo.model = modelMatrix;
    ubo.norm = glm::transpose(glm::inverse(viewMatrix * ubo.model));

    return ubo;
}

void UniformManager::createUniformBuffer() 
{
    VkDeviceSize bufferSize = sizeof(UniformManager::StaticUbo);
    Utils::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
}

void UniformManager::createDynamicUniformBuffer(size_t &dynamicAlignment, size_t objectCount)
{
    // Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = DeviceManager::instance().getProperties().limits.minUniformBufferOffsetAlignment;
    
    dynamicAlignment = sizeof(UniformManager::DynamicUbo);
    
    if (minUboAlignment > 0)
    {
        dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    VkDeviceSize bufferSize = objectCount * dynamicAlignment;
    Utils::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, dynamicUniformBuffer, dynamicMemory);
}

void UniformManager::cleanupUniformBuffers()
{
    VkDevice device = DeviceManager::instance().getDevice();

    vkDestroyBuffer(device, dynamicUniformBuffer, nullptr);
    vkFreeMemory(device, dynamicMemory, nullptr);
    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformBufferMemory, nullptr);
}