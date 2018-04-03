#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>

class Vertex
{
public:

    // Vertex attributes
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoord;

    // Specifies rate at which vertex data should be passed to the vertex shader
    static VkVertexInputBindingDescription getBindingDescription();

    // Specifies how to extract a given vertex attribute from vertex data
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();

    // Comparison operator
    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && normal == other.normal && color == other.color && texCoord == other.texCoord;
    }
};