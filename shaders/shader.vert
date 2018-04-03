#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 norm;

    vec3 cameraPos;
    vec3 lightPos;
    vec3 lightDiffuseColor;
} ubo;

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

// Fragment attributes
layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec2 fragTexCoord;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    vec4 worldPos = ubo.model * ubo.view * vec4(inPosition, 1.0);
    
    fragPosition = vec3(worldPos) / worldPos.w;
    fragNormal = vec3(ubo.norm * vec4(inNormal, 0.0));
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    gl_Position = ubo.proj * worldPos;
}