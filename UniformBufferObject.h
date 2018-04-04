#pragma once

#include <glm/glm.hpp>

class UniformBufferObject
{
public:

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

    static DynamicUbo createDynamicUbo(glm::mat4 modelMatrix, glm::mat4 viewMatrix)
    {
        DynamicUbo ubo = {};

        // Set model and normal matrices
        ubo.model = modelMatrix;
        ubo.norm = glm::transpose(glm::inverse(ubo.model * viewMatrix));

        return ubo;
    }

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 norm;
};