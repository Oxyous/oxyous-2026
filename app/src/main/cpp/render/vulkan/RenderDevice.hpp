//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#ifndef OXYOUS_2026_RENDERDEVICE_HPP
#define OXYOUS_2026_RENDERDEVICE_HPP

#include "../../includes.hpp"
#include "../../System/OGSingleton.hpp"
#include <vulkan/vulkan_core.h>

class RenderDevice {
public:

    /* Initialize Vulkan Render Device */
    virtual bool initialize(VkInstance& instance, VkSurfaceKHR& surface);

    /*  */
    virtual void destroy();

    /*  */
    virtual VkDevice getDevice() { return m_device; }

    /*  */
    virtual VkPhysicalDevice getPhysicalDevice() { return m_physicalDevice; }

    /*  */
    virtual VkQueue getGraphicsQueue() { return m_graphicsQueue; }

    /* */
    virtual VkQueue getPresentQueue() { return m_presentQueue; }

    /* */
    virtual VkQueue getComputeQueue() { return m_computeQueue; }

    /* */
    virtual VkCommandPool getPrimaryCommandPool() { return m_primaryCommandPool; }

    /* */

protected:
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    VkDevice m_device{VK_NULL_HANDLE};
    VkQueue m_graphicsQueue{VK_NULL_HANDLE};
    VkQueue m_presentQueue{VK_NULL_HANDLE};
    VkQueue m_computeQueue{VK_NULL_HANDLE};

    uint32_t m_graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t m_presentQueueFamilyIndex = UINT32_MAX;
    uint32_t m_computeQueueFamilyIndex = UINT32_MAX;
private:
    VkInstance m_instance;
    VkCommandPool m_primaryCommandPool;

    static VkPhysicalDeviceMemoryProperties m_memoryProperties;

    VkPhysicalDeviceFeatures m_gpuFeatures;
    VkPhysicalDeviceFeatures m_enabledFeatures;
    VkPhysicalDeviceProperties m_gpuProperties;

    /* Get Family Queue indices */
    void getQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR& surface);

};

#define RENDER_DEVICE OGSingleton<RenderDevice>::getInstance()

#endif //OXYOUS_2026_RENDERDEVICE_HPP
