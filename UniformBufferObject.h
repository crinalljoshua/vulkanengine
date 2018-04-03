#pragma once

#include <glm/glm.hpp>

class UniformBufferObject
{
public:

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 norm;
};