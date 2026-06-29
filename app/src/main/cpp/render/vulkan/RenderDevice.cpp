//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#include "RenderDevice.hpp"
#include "RenderFramework.hpp"
#include "DescriptorCache.hpp"

VkPhysicalDeviceMemoryProperties RenderDevice::m_memoryProperties;

bool RenderDevice::initialize(VkInstance &instance, VkSurfaceKHR &surface) {
    uint32_t gpuCount = 0;

    vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);

    if (gpuCount == 0) {
        aout << "Error: No Vulkan compatible GPU found" << std::endl;
        return false;
    }

    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());

    // Find a GPU that supports graphics and presentation
    bool foundGpu = false;
    for (const auto &gpu: physicalDevices) {
        // Get Family Properties for this GPU
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilies.data());

        bool supportsGraphics = false;
        bool supportsPresent = false;
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                supportsGraphics = true;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &presentSupport);
            if (presentSupport) {
                supportsPresent = true;
            }
        }

        if (supportsGraphics && supportsPresent) {
            m_physicalDevice = gpu;
            foundGpu = true;
            break;
        }
    }

    if (!foundGpu) {
        aout << "Error: No suitable Vulkan GPU found (graphics + present support required)"
             << std::endl;
        return false;
    }

    // Get the memory properties of the physical device
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);

    // Get Gpu Properties
    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_gpuProperties);

    // Get Family Properties
    getQueueFamilyIndices(m_physicalDevice, surface);

    // Debug output
    aout << "GPU Name: " << m_gpuProperties.deviceName << std::endl;
    aout << "GPU Type: " << m_gpuProperties.deviceType << std::endl;
    aout << "Graphics Queue Family Index: " << m_graphicsQueueFamilyIndex << std::endl;
    aout << "Present Queue Family Index: " << m_presentQueueFamilyIndex << std::endl;
    aout << "Compute Queue Family Index: " << m_computeQueueFamilyIndex << std::endl;

    vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_gpuFeatures);

    // Check if the physical device supports the required features
    if (!m_gpuFeatures.geometryShader) {
        aout
                << "Warning: Physical device does not support geometry shaders. This may limit some effects."
                << std::endl;
    }

    // Device Layer Properties
    uint32_t deviceLayerCount = 0;
    vkEnumerateDeviceLayerProperties(m_physicalDevice, &deviceLayerCount, nullptr);

    std::vector<VkLayerProperties> deviceLayers(deviceLayerCount);
    vkEnumerateDeviceLayerProperties(m_physicalDevice, &deviceLayerCount, deviceLayers.data());

    std::vector<const char *> enabledLayers;

    for (const auto &layer: deviceLayers) {
        if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            enabledLayers.push_back(layer.layerName);
            aout << "Enabling device layer: " << layer.layerName << std::endl;
        }
    }

    const char *deviceExtensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<uint32_t> uniqueQueueFamilies;

    if (m_graphicsQueueFamilyIndex != UINT32_MAX)
        uniqueQueueFamilies.push_back(m_graphicsQueueFamilyIndex);

    if (m_presentQueueFamilyIndex != UINT32_MAX) {
        bool found = false;
        for (uint32_t f: uniqueQueueFamilies) {
            if (f == m_presentQueueFamilyIndex) {
                found = true;
                break;
            }
        }
        if (!found) uniqueQueueFamilies.push_back(m_presentQueueFamilyIndex);
    }

    if (m_computeQueueFamilyIndex != UINT32_MAX) {
        bool found = false;
        for (uint32_t f: uniqueQueueFamilies) {
            if (f == m_computeQueueFamilyIndex) {
                found = true;
                break;
            }
        }
        if (!found) uniqueQueueFamilies.push_back(m_computeQueueFamilyIndex);
    }

    if (uniqueQueueFamilies.empty() || m_graphicsQueueFamilyIndex == UINT32_MAX ||
        m_presentQueueFamilyIndex == UINT32_MAX) {
        aout << "Error: Necessary queue families not found" << std::endl;
        return false;
    }

    float queuePriority = 1.0f;
    for (uint32_t queueFamily: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    /* Descriptor indexing feature */

    /* Check for support for descriptor indexing */
    VkPhysicalDeviceDescriptorIndexingFeatures features{};
    features.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    features.runtimeDescriptorArray = VK_TRUE;
    features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    features.descriptorBindingPartiallyBound = VK_TRUE;
    features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;


    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features;
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);

    if (!features.descriptorBindingPartiallyBound) {
        aout << "Error: Descriptor indexing not supported" << std::endl;
        return false;
    }

    if (!features.descriptorBindingSampledImageUpdateAfterBind) {
        aout << "Error: Descriptor indexing not supported" << std::endl;
        return false;
    }

    if (!features.runtimeDescriptorArray) {
        aout << "Error: Descriptor indexing not supported" << std::endl;
        return false;
    }

    if (!features.descriptorBindingVariableDescriptorCount) {
        aout << "Error: Descriptor indexing not supported" << std::endl;
        return false;
    }

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {};
    descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

    /* Create Vulkan Device */
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceCreateInfo.pNext = &descriptorIndexingFeatures;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    if (m_gpuFeatures.geometryShader) {
        deviceFeatures.geometryShader = VK_TRUE;
    }
    deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
    deviceFeatures.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);

    if (result != VK_SUCCESS) {
        aout << "Error: Failed to create logical device, error code: " << result << std::endl;
        return false;
    }

    vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentQueueFamilyIndex, 0, &m_presentQueue);

    if (m_computeQueueFamilyIndex != UINT32_MAX) {
        vkGetDeviceQueue(m_device, m_computeQueueFamilyIndex, 0, &m_computeQueue);
    }

    /* Allocate Command Pool for Initialization Commands */
    if (!RenderFramework::allocateCommandPool(m_device, &m_primaryCommandPool,
                                              m_graphicsQueueFamilyIndex)) {
        aout << "Error: Failed to create command pool" << std::endl;
        return false;
    }



    return true;
}

void RenderDevice::destroy() {
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
        vkDestroyCommandPool(m_device, m_primaryCommandPool, nullptr);
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
        m_primaryCommandPool = VK_NULL_HANDLE;
    }
}

void RenderDevice::getQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR &surface) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_graphicsQueueFamilyIndex = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

        if (presentSupport) {
            m_presentQueueFamilyIndex = i;
        }

        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            m_computeQueueFamilyIndex = i;
        }
    }
}

