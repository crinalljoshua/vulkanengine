#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const vec3 lightPos = vec3(2.0,0.0,-2.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightPower = 5.0;
const vec3 ambientTerm = vec3(0.1, 0.1, 0.1);
const vec3 diffuseColor = vec3(0.5, 0.5, 0.5);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;

vec3 getSpecularTerm(vec3 lightDir, vec3 normal, float lightDist)
{
    vec3 viewDir = normalize(-fragPosition);

    vec3 halfDir = normalize(lightDir + viewDir);
    float specularAngle = max(dot(halfDir, normal), 0.0);
    float specular = pow(specularAngle, shininess);

    return specColor * specular * lightColor * lightPower / lightDist;
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = lightPos - fragPosition;
    float lightDist = length(lightDir);

    lightDist = lightDist * lightDist;
    lightDir = normalize(lightDir);

    float lambertian = max(dot(lightDir, normal), 0.0);

    vec3 diffuseTerm = diffuseColor * lambertian * lightColor * lightPower / lightDist;

    vec3 specularTerm = vec3(0.0, 0.0, 0.0);
    if (lambertian > 0.0)
    {
        specularTerm = getSpecularTerm(lightDir, normal, lightDist);
    }

    outColor = vec4((ambientTerm + diffuseTerm + specularTerm), 1.0);
    // outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}