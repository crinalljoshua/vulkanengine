#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include "Utils.h"
#include "Vertex.h"
#include "QueueFamilyIndices.h"
#include "SwapchainSupportDetails.h"
#include "UniformManager.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <cstring>
#include <set>
#include <limits>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>

const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// Create debug report callback for validation layers
VkResult CreateDebugReportCallbackEXT(
    VkInstance instance, 
    const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDebugReportCallbackEXT* pCallback) 
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

    if (func != nullptr) 
    {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } 
    else 
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Destroy debug report callback on exit
void DestroyDebugReportCallbackEXT(
    VkInstance instance,
    VkDebugReportCallbackEXT callback,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

    if (func != nullptr)
    {
        func(instance, callback, pAllocator);
    }
}

namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ 
                    (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                    (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

// class DeviceManager;

class VulkanApplication
{
public:

    void run()
    {
        initWindow();
        initVulkan();

        prevFrameTime = std::chrono::high_resolution_clock::now();

        mainLoop();
        cleanup();
    }

private:

    GLFWwindow* window;
    VkInstance instance;

    VkDebugReportCallbackEXT callback;

    // Window surface
    VkSurfaceKHR surface;

    // Device queues
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    // Render pass
    VkRenderPass renderPass;

    // Descriptor set
    VkDescriptorSetLayout descriptorSetLayout;

    size_t dynamicAlignment;

    // Pipeline objects
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    // Command pool/buffers
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // Semaphores
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    // Vertex/index buffer & buffer memory
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Descriptor pool/set
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    // Textures
    VkImage mainTextureImage;
    VkDeviceMemory mainTextureImageMemory;
    VkImage groundTextureImage;
    VkDeviceMemory groundTextureImageMemory;

    // Texture image views + sampler
    VkImageView mainTextureImageView;
    VkImageView groundTextureImageView;
    VkSampler textureSampler;

    // Depth buffering
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::chrono::high_resolution_clock::time_point prevFrameTime;
    std::chrono::high_resolution_clock::time_point currentFrameTime;

    struct Object
    {
        uint32_t firstIndex;
        uint32_t indexCount;
    };

    std::vector<Object> objects;

    struct Camera
    {
        glm::vec3 position = glm::vec3(-4.0f, 4.0f, -8.0f);
        glm::vec3 viewDir = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 center = position + viewDir;
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);

        float moveSpeedFactor = 8.0;

        void updateCamera(GLFWwindow* window, float deltaTime)
        {
            glm::vec3 movement;

            position += velocity;
            center = position + viewDir;

            if (glfwGetKey(window, GLFW_KEY_W) != GLFW_RELEASE)
            {
                movement += glm::vec3(0.0f, 0.0f, 1.0f);
            }

            if (glfwGetKey(window, GLFW_KEY_A) != GLFW_RELEASE)
            {
                movement += glm::vec3(1.0f, 0.0f, 0.0f);
            }

            if (glfwGetKey(window, GLFW_KEY_S) != GLFW_RELEASE)
            {
                movement += glm::vec3(0.0f, 0.0f, -1.0f);
            }

            if (glfwGetKey(window, GLFW_KEY_D) != GLFW_RELEASE)
            {
                movement += glm::vec3(-1.0f, 0.0f, 0.0f);
            }

            if (glfwGetKey(window, GLFW_KEY_Q) != GLFW_RELEASE)
            {
                movement += glm::vec3(0.0f, -1.0f, 0.0f);
            }

            if (glfwGetKey(window, GLFW_KEY_E) != GLFW_RELEASE)
            {
                movement += glm::vec3(0.0f, 1.0f, 0.0f);
            }

            glm::normalize(movement);

            float moveFactor = moveSpeedFactor * deltaTime;
            movement *= moveFactor;

            position += movement;
        }
    };

    Camera camera; 

    void initWindow()
    {
        // Initialise glfw
        glfwInit();

        // Set window properties
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        // Create GLFWwindow
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetWindowSizeCallback(window, VulkanApplication::onWindowResized);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugCallback();
        createSurface();

        // camera = new Camera(window);

        DeviceManager::instance().pickPhysicalDevice(instance, surface);
        DeviceManager::instance().createLogicalDevice(surface, graphicsQueue, presentQueue, enableValidationLayers, validationLayers);

        SwapchainManager::instance().createSwapchain(surface, window);
        SwapchainManager::instance().createImageViews();
        
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createDepthResources();

        SwapchainManager::instance().createFramebuffers(depthImageView, renderPass);
        
        createTextureImage("textures/texture.jpg", mainTextureImage, mainTextureImageMemory);
        createTextureImageView(mainTextureImage, mainTextureImageView);
        createTextureImage("textures/ground.png", groundTextureImage, groundTextureImageMemory);
        createTextureImageView(groundTextureImage, groundTextureImageView);
        createTextureSampler();

        loadModel("models/sphere.obj");
        loadModel("models/plane.obj");

        createVertexBuffer();
        createIndexBuffer();

        UniformManager::instance().createUniformBuffer();
        UniformManager::instance().createDynamicUniformBuffer(dynamicAlignment, objects.size());

        createDescriptorPool();
        createDescriptorSet(descriptorSet);
        createCommandBuffers();
        createSemaphores();
    }

    void setupDebugCallback()
    {
        if (!enableValidationLayers) return;

        // Create debug report callback info
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = debugCallback;

        // Attempt to create debug report callback
        if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to set up debug callback.");
        }
    }

    // Create window surface using GLFW
    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to create window surface");
        }
    }

    // Recreate swap chain for window resizing
    void recreateSwapChain()
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        
        if (width == 0 || height == 0) return;
        
        vkDeviceWaitIdle(DeviceManager::instance().getDevice());

        cleanupSwapChain();

        SwapchainManager::instance().createSwapchain(surface, window);
        SwapchainManager::instance().createImageViews();

        createRenderPass();
        createGraphicsPipeline();
        createDepthResources();

        SwapchainManager::instance().createFramebuffers(depthImageView, renderPass);

        createCommandBuffers();
    }

    void createRenderPass()
    {
        // Colour buffer attachment info
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = SwapchainManager::instance().getImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Reference to colour attachment
        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(DeviceManager::instance().getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to create render pass");
        }
    }

    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding staticUboLayoutBinding = {};
        staticUboLayoutBinding.binding = 0;
        staticUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        staticUboLayoutBinding.descriptorCount = 1;
        staticUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        staticUboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding dynamicUboLayoutBinding = {};
        dynamicUboLayoutBinding.binding = 1;
        dynamicUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dynamicUboLayoutBinding.descriptorCount = 1;
        dynamicUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        dynamicUboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 2;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding textureLayoutBinding = {};
        textureLayoutBinding.binding = 3;
        textureLayoutBinding.descriptorCount = 2; // number of textures
        textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        textureLayoutBinding.pImmutableSamplers = nullptr;
        textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 4> bindings = { staticUboLayoutBinding, dynamicUboLayoutBinding, samplerLayoutBinding, textureLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(DeviceManager::instance().getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to create descriptor set layout");
        }
    }

    void createGraphicsPipeline()
    {
        // Read in vertex and fragment shader bytecode
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        // Use shader bytecode to create shader modules
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        // Populate vertex shader module parameter struct
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Populate fragment shader module parameter struct
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // Create array to hold vertex & fragment shader module stage info
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        // Populate vertex input info struct
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Populate input assembly struct
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkExtent2D swapchainExtent = SwapchainManager::instance().getExtent();

        // Viewport settings
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapchainExtent.width;
        viewport.height = (float) swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Viewport scissor for rasteriser pixel discarding, set up to cover entire framebuffer
        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = swapchainExtent;

        // Use viewport and scissor to set up viewport state
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // Rasteriser settings
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        // Multisampling parameters (i.e. for anti-aliasing)
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        // Color blend parameters
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(int);
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VkDevice device = DeviceManager::instance().getDevice();

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to create pipeline layout");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr; // Optional
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to creat graphics pipeline");
        }

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
    }

    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices::findQueueFamilies(DeviceManager::instance().getPhysicalDevice(), surface);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
        poolInfo.flags = 0;

        if (vkCreateCommandPool(DeviceManager::instance().getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to create command pool");
        }
    }

    void createDepthResources()
    {
        VkFormat depthFormat = findDepthFormat();

        VkExtent2D swapchainExtent = SwapchainManager::instance().getExtent();

        createImage(swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        VkDevice device = DeviceManager::instance().getDevice();

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to create image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = Utils::findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to allocate image memory");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void createTextureImage(std::string filepath, VkImage &image, VkDeviceMemory &imageMemory)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        // 4 bytes per pixel
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels)
        {
            throw std::runtime_error("Error: Failed to load texture image");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        VkDevice device = DeviceManager::instance().getDevice();

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);
    
        transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createTextureImageView(VkImage &textureImage, VkImageView &dstImageView)
    {
        dstImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void createTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE; // Potentially condition
        samplerInfo.maxAnisotropy = 16;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(DeviceManager::instance().getDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to create texture sampler");
        }
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(format)) 
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } 
        else 
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("Error: Unsupported layout transition");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkDevice device = DeviceManager::instance().getDevice();

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to create vertex buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = Utils::findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to allocate vertex buffer memory");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void loadModel(std::string filepath)
    {
        // Load using tinyobj containers
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;

        // Load obj from file
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath.c_str())) 
        {
            throw std::runtime_error(err);
        }

        // Map vertex to index
        std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

        Object object;

        object.firstIndex = static_cast<uint32_t>(indices.size());

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices) 
            {
                Vertex vertex = {};

                vertex.pos = { attrib.vertices[3 * index.vertex_index + 0], // Single strip values - index * 3
                               attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2] };

                vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                                 attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]};

                vertex.texCoord = { attrib.texcoords.at(2 * index.texcoord_index + 0),
                                    1.0f - attrib.texcoords.at(2 * index.texcoord_index + 1) };

                vertex.color = { 1.0f, 1.0f, 1.0f };

                if (uniqueVertices.count(vertex) == 0)
                {
                    // Push current vector size (ie index) to uniqueVertices and push vertex to vertex array
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                // Retrieve unique vertex index from map
                indices.push_back(uniqueVertices[vertex]);
            }
        }

        object.indexCount = static_cast<uint32_t>(indices.size());
        objects.push_back(object);
    }

    void createVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        VkDevice device = DeviceManager::instance().getDevice();

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        VkDevice device = DeviceManager::instance().getDevice();

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    // void createUniformBuffer() 
    // {
    //     VkDeviceSize bufferSize = sizeof(UniformManager::StaticUbo);
    //     createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
    // }

    // void createDynamicUniformBuffer()
    // {
    //     // Calculate required alignment based on minimum device offset alignment
    //     size_t minUboAlignment = DeviceManager::instance().getProperties().limits.minUniformBufferOffsetAlignment;
        
    //     dynamicAlignment = sizeof(UniformManager::DynamicUbo);
        
    //     if (minUboAlignment > 0)
    //     {
    //         dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
    //     }

    //     VkDeviceSize bufferSize = objects.size() * dynamicAlignment;
    //     createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, dynamicUniformBuffer, dynamicMemory);
    // }

    void createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 4> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSizes[0].descriptorCount = 1;

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = 1;

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSizes[2].descriptorCount = 1;

        poolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[3].descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(DeviceManager::instance().getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to create descriptor pool");
        }
    }

    void createDescriptorSet(VkDescriptorSet &descriptorSet)
    {
        VkDescriptorSetLayout layouts[] = {descriptorSetLayout, descriptorSetLayout};

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layouts;

        VkDevice device = DeviceManager::instance().getDevice();

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to allocate descriptor set");
        }

        VkDescriptorBufferInfo dynamicBufferInfo = {};
        dynamicBufferInfo.buffer = UniformManager::instance().getDynamicUniformBuffer();
        dynamicBufferInfo.offset = 0;
        dynamicBufferInfo.range = sizeof(UniformManager::DynamicUbo);

        VkDescriptorBufferInfo staticBufferInfo = {};
        staticBufferInfo.buffer = UniformManager::instance().getCoherentUniformBuffer();
        staticBufferInfo.offset = 0;
        staticBufferInfo.range = sizeof(UniformManager::StaticUbo);

        VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = textureSampler;

        VkDescriptorImageInfo mainImageInfo = {};
        mainImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mainImageInfo.imageView = mainTextureImageView;
        mainImageInfo.sampler = textureSampler;

        VkDescriptorImageInfo groundImageInfo = {};
        groundImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        groundImageInfo.imageView = groundTextureImageView;
        groundImageInfo.sampler = textureSampler;

        VkDescriptorImageInfo imageInfo[2];
        imageInfo[0] = mainImageInfo;
        imageInfo[1] = groundImageInfo;

        std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &dynamicBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &staticBufferInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSet;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &samplerInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptorSet;
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[3].descriptorCount = 2; // Number of textures
        descriptorWrites[3].pImageInfo = imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(SwapchainManager::instance().getFramebufferSize());

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(DeviceManager::instance().getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to allocate command buffers");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) 
        {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

            std::array<VkClearValue, 2> clearValues = {};
            clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
            clearValues[1].depthStencil = { 1.0f, 0 };

            std::vector<VkFramebuffer> swapchainFramebuffers = SwapchainManager::instance().getFramebuffers();
            VkExtent2D swapchainExtent = SwapchainManager::instance().getExtent();

            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapchainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapchainExtent;
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};

            // Bind vertex and index buffers
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            // Loop through objects and bind descriptor set to draw call
            // This uses a dynamic offset to select what data from the uniform buffer each draw call requires
            for (size_t j = 0; j < objects.size(); j++)
            {
                uint32_t dynamicOffset = j * static_cast<uint32_t>(dynamicAlignment);

                int index = j;
                vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), (void*)&index);
                vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, &dynamicOffset);

                // Draw single object using vertex count and object first index
                vkCmdDrawIndexed(commandBuffers[i], objects[j].indexCount, 1, objects[j].firstIndex, 0, 0);
            }
            
            // End render pass and command buffer
            vkCmdEndRenderPass(commandBuffers[i]);
            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Error: Failed to record command buffer");
            }
        }
    }

    void createSemaphores()
    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkDevice device = DeviceManager::instance().getDevice();

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) 
        {

            throw std::runtime_error("Error: Failed to create semaphores");
        }
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }

        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
    {
        VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
            else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                bestMode = availablePresentMode;
            }
        }

        return bestMode;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetWindowSize(window, &width, &height);

            VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;

        if (vkCreateShaderModule(DeviceManager::instance().getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to create shader module");
        }

        return shaderModule;
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;

        if (vkCreateImageView(DeviceManager::instance().getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to create texture image view");
        }

        return imageView;
    }

    VkCommandBuffer beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(DeviceManager::instance().getDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates) 
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(DeviceManager::instance().getPhysicalDevice(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
            {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
            {
                return format;
            }
        }

        throw std::runtime_error("Error: Failed to find supported format");
    }

    VkFormat findDepthFormat()
    {
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(DeviceManager::instance().getDevice(), commandPool, 1, &commandBuffer);
    }

    void mainLoop()
    {
        // Poll events while window open
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            currentFrameTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentFrameTime - prevFrameTime).count();

            camera.updateCamera(window, time);
            updateUniformBuffer();
            drawFrame();

            prevFrameTime = currentFrameTime;
        }

        vkDeviceWaitIdle(DeviceManager::instance().getDevice());
    }

    void updateUniformBuffer()
    {
        // Get elapsed time per frame for time-based transforms
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        VkExtent2D swapchainExtent = SwapchainManager::instance().getExtent();

        // View matrix
        glm::mat4 view = glm::lookAt(camera.position,  // Camera position
                                     camera.center,  // Coordinates to look at
                                     camera.up); // Up vector

        // Projection matrix - 45 degree fov, aspect ratio and near/far planes
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 
                                          swapchainExtent.width / (float)swapchainExtent.height, 
                                          0.0001f, 
                                          1000.0f);
        // GLM designed for OpenGL - must flip Y coordinate of clip coords
        proj[1][1] *= -1;


        UniformManager::DynamicUbo dynamicUbos[2];

        // Model matrix - rotate main model 45 degrees per second
        UniformManager::DynamicUbo ubo1 = UniformManager::createDynamicUbo(glm::rotate(glm::mat4(1.0f), 
                                                                                       time * glm::radians(90.0f), 
                                                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f)), 
                                                                           view);

        // Model matrix - translate ground plane below main model
        UniformManager::DynamicUbo ubo2 = UniformManager::createDynamicUbo(glm::translate(glm::mat4(1.0f), 
                                                                                          glm::vec3(0.0f, -3.0f, 0.0f)), 
                                                                           view);

        dynamicUbos[0] = ubo1;
        dynamicUbos[1] = ubo2;

        // Get device handle and buffer memories
        VkDevice device = DeviceManager::instance().getDevice();
        VkDeviceMemory dynamicMemory = UniformManager::instance().getDynamicMemory();
        VkDeviceMemory uniformBufferMemory = UniformManager::instance().getCoherentMemory();

        // Set up memory range object for flushing specific parts of dynamic uniform buffer
        VkMappedMemoryRange memoryRange;
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.pNext = nullptr;
        memoryRange.memory = dynamicMemory;
        memoryRange.size = sizeof(UniformManager::DynamicUbo);

        // Update dynamic uniform buffer data
        for (int i = 0; i < objects.size(); i++)
        {
            // Calculate memory range offset
            memoryRange.offset = i * dynamicAlignment;

            // Map memory and copy data to mapping
            void* data;
            vkMapMemory(device, dynamicMemory, i * dynamicAlignment, dynamicAlignment, 0, &data);
            memcpy(data, &dynamicUbos[i], sizeof(dynamicUbos[i]));

            // Flush modified memory ranges and unmap memory
            vkFlushMappedMemoryRanges(device, 1, &memoryRange);
            vkUnmapMemory(device, dynamicMemory);
        }
        
        // Update static uniform buffer data
        UniformManager::StaticUbo ubo;

        ubo.view = view;
        ubo.proj = proj;

        void* data;
        vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformBufferMemory);
    }

    void drawFrame()
    {
        uint32_t imageIndex;
        VkDevice device = DeviceManager::instance().getDevice();
        VkSwapchainKHR swapchain = SwapchainManager::instance().getSwapchain();
        VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) 
        {
            recreateSwapChain();
            return;
        } 
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
        {
            throw std::runtime_error("Error: Failed to acquire swap chain image");
        }

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to submit draw command buffer");
        }

        VkSwapchainKHR swapChains[] = {swapchain};

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
        {
            recreateSwapChain();
        } 
        else if (result != VK_SUCCESS) 
        {
            throw std::runtime_error("Error: Failed to acquire swap chain image");
        }

        vkQueueWaitIdle(presentQueue);
    }

    void cleanup()
    {
        VkDevice device = DeviceManager::instance().getDevice();

        cleanupSwapChain();

        // Destroy descriptor set pool/layout
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        // Destroy uniform buffers
        UniformManager::instance().cleanupUniformBuffers();

        // Destroy index buffer
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        // Destroy vertex buffer
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        // Destroy texture sampler
        vkDestroySampler(device, textureSampler, nullptr);

        // Destroy image view
        vkDestroyImageView(device, mainTextureImageView, nullptr);
        vkDestroyImageView(device, groundTextureImageView, nullptr);

        // Destroy textures
        vkDestroyImage(device, mainTextureImage, nullptr);
        vkFreeMemory(device, mainTextureImageMemory, nullptr);
        vkDestroyImage(device, groundTextureImage, nullptr);
        vkFreeMemory(device, groundTextureImageMemory, nullptr);

        // Destroy debug report callback on cleanup
        DestroyDebugReportCallbackEXT(instance, callback, nullptr);

        // Destroy command pool
        vkDestroyCommandPool(device, commandPool, nullptr);

        // Destroy semaphores
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

        // Destroy logical device
        vkDestroyDevice(device, nullptr);

        // Destroy window surface
        vkDestroySurfaceKHR(instance, surface, nullptr);

        // Destroy Vulkan instance, ignore registering callback
        vkDestroyInstance(instance, nullptr);

        // Destroy GLFWwindow & terminate GLFW
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void cleanupSwapChain()
    {
        VkDevice device = DeviceManager::instance().getDevice();

        VkSwapchainKHR swapchain = SwapchainManager::instance().getSwapchain();
        std::vector<VkFramebuffer> swapchainFramebuffers = SwapchainManager::instance().getFramebuffers();
        std::vector<VkImageView> swapchainImageViews = SwapchainManager::instance().getImageViews();

        for (auto framebuffer : swapchainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        // Destroy swapchain image views
        for (auto imageView : swapchainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        // Destroy depth resources
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);

        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

        // Destroy graphics pipeline
        vkDestroyPipeline(device, graphicsPipeline, nullptr);

        // Destroy pipeline layout
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        // Destroy render pass
        vkDestroyRenderPass(device, renderPass, nullptr);

        // Destroy swapchain
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }

    bool checkValidationLayerSupport()
    {
        // Initialise validation layer count
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);


        // Populate available layers vector
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // Check required validation layers are available
        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    bool hasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        // Get required GLFW extensions
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // Populate required glfw extension vector
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // Enable debug report extension (optional)
        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        return extensions;
    }

    void createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers requested, but not available");
        }

        // Acquire VkApplication info
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Create VkInstance using app info
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Add validation layer data to instance creation info
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        // Get required extensions and add to instance creation info
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        std::cout << std::endl;
        std::cout << "Required extensions:" << std::endl;
        for (size_t i=0; i<extensions.size(); i++)
        {
            std::cout << "\t" << extensions[i] << std::endl;
        }

        std::cout << std::endl;

        // Initialise Vulkan extension count
        uint32_t extensionCount = 0;

        // Poll number of extensions by not specifying extension properties data structure
        // Once the number has been found, re-poll and specify vector
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> vkExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkExtensions.data());

        // Print available extensions
        std::cout << "Available Vulkan extensions:" << std::endl;
        for (const auto& extension : vkExtensions)
        {
            std::cout << "\t" << extension.extensionName << std::endl;
        }

        std::cout << std::endl;

        // Create Vulkan instance, throw error if creation returns failure
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to create instance");
        }

        std::cout << "Vulkan instance created." << std::endl;
    }

    static void onWindowResized(GLFWwindow* window, int width, int height)
    {
        VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        // Open file and start at end
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Error: Failed to open file");
        }

        // Use end position to get size of file for buffer allocation
        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        // Seek back to beginning of file, read data and close
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    // Debug callback returns messages from validation layers
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugReportFlagsEXT flags, 
        VkDebugReportObjectTypeEXT objType, 
        uint64_t obj, 
        size_t location, 
        int32_t code, 
        const char* layerPrefix, 
        const char* msg, 
        void* userData)
    {
        std::cerr << "Validation layer: " << msg << std::endl;

        return VK_FALSE;
    }
};

int main()
{
    VulkanApplication app;

    try
    {
        app.run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
