#include "DeviceManager.h"

DeviceManager& DeviceManager::instance()
{
    static DeviceManager instance;

    return instance;
}

VkDevice DeviceManager::getDevice()
{
    return device;
}

VkPhysicalDevice DeviceManager::getPhysicalDevice()
{
    return physicalDevice;
}

VkPhysicalDeviceProperties DeviceManager::getProperties()
{
    return properties;
}

void DeviceManager::pickPhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface)
{
    // Enumerate device count
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("Error: Failed to find GPUs with Vulkan support");
    }

    // Populate physical device vector
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Choose first suitable physical device
    for (const auto& potentialDevice : devices)
    {
        if (isDeviceSuitable(potentialDevice, surface))
        {
            physicalDevice = potentialDevice;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Error: Failed to find a suitable GPU");
    }
}

void DeviceManager::createLogicalDevice(VkSurfaceKHR& surface, VkQueue& graphicsQueue, VkQueue& presentQueue, bool enableValidationLayers, const std::vector<const char*>& validationLayers)
{
    // Acquire device queue family indices
    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);

    // Set up create info vector and unique queue family set
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    // Populate device queue creation info structs
    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Device features struct
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    // Device creation info
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Set validation layer info if applicable
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    // Use info to create physical device
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: Failed to create logical device");
    }

    // Get device queues for graphics/present families
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

// Ensure a given device supports required queue families
bool DeviceManager::isDeviceSuitable(VkPhysicalDevice potentialDevice, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(potentialDevice, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(potentialDevice);

    bool swapchainAdequate = false;

    if (extensionsSupported)
    {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(potentialDevice, surface);
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(potentialDevice, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
}

bool DeviceManager::checkDeviceExtensionSupport(VkPhysicalDevice potentialDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(potentialDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(potentialDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapchainSupportDetails DeviceManager::querySwapchainSupport(VkPhysicalDevice potentialDevice, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(potentialDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(potentialDevice, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(potentialDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(potentialDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(potentialDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}