//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#include "Swapchain.hpp"
#include "RenderDevice.hpp"
#include "../../includes.hpp"
#include "RenderFramework.hpp"

Swapchain::Swapchain() = default;

Swapchain::~Swapchain() = default;

bool Swapchain::initialize(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice device,
                           uint32_t &width, uint32_t &height) {

    m_surface = surface;
    m_physicalDevice = physicalDevice;
    m_device = device;

    uint32_t presentCount = UINT32_MAX;
    auto err = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentCount,
                                                         nullptr);
    if (err != VK_SUCCESS) {
        aout << "Error: Failed to get present mode" << std::endl;
        return false;
    }

    m_presentModes.resize(presentCount);

    err = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentCount,
                                                    m_presentModes.data());
    if (err != VK_SUCCESS) {
        aout << "Error: Failed to get present modes" << std::endl;
        return false;
    }

    m_activePresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto p: m_presentModes) {
        if (p == VK_PRESENT_MODE_MAILBOX_KHR) {
            m_activePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    /* Get Presentation Capabilities  */
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface,
                                                    &m_surfaceCapabilities);
    if (err != VK_SUCCESS) {
        aout << "Error: Failed to get surface capabilities" << std::endl;
        return false;
    }

    /* Get swapchain images from surface*/
    m_imageCount = m_surfaceCapabilities.minImageCount + 1;
    if ((m_surfaceCapabilities.maxImageCount > 0) &&
        (m_imageCount > m_surfaceCapabilities.maxImageCount)) {
        m_imageCount = m_surfaceCapabilities.maxImageCount;
    }

    /* Get Swapchain size */
    if (m_surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
        m_extent = {width, height};

        if (m_extent.width < m_surfaceCapabilities.minImageExtent.width) {
            m_extent.width = m_surfaceCapabilities.minImageExtent.width;
        } else if (m_extent.width > m_surfaceCapabilities.maxImageExtent.width) {
            m_extent.width = m_surfaceCapabilities.maxImageExtent.width;
        }

        if (m_extent.height < m_surfaceCapabilities.minImageExtent.height) {
            m_extent.height = m_surfaceCapabilities.minImageExtent.height;
        } else if (m_extent.height > m_surfaceCapabilities.maxImageExtent.height) {
            m_extent.height = m_surfaceCapabilities.maxImageExtent.height;
        }
    } else {
        m_extent = m_surfaceCapabilities.currentExtent;
    }

    /* Set Width and Height*/
    m_width = m_extent.width;
    m_height = m_extent.height;

    m_preTransform = m_surfaceCapabilities.currentTransform;

    /* Get Swapchain image formats */
    uint32_t formatCount = 0;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    if (err != VK_SUCCESS || formatCount < 1) {
        aout << "Error: Failed to get surface formats" << std::endl;
        return false;
    }

    m_surfaceFormats.resize(formatCount);
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount,
                                               m_surfaceFormats.data());
    if (err != VK_SUCCESS) {
        aout << "Error: Failed to get surface formats" << std::endl;
        return false;
    }

    if (formatCount == 1 && (m_surfaceFormats[0].format == VK_FORMAT_UNDEFINED)) {
        m_imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    } else {
        m_imageFormat = m_surfaceFormats[0].format;
    }

    /* Create Swapchain */

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = m_surface;
    swapchainCreateInfo.minImageCount = m_imageCount;
    swapchainCreateInfo.imageFormat = m_imageFormat;
    swapchainCreateInfo.imageColorSpace = m_surfaceFormats[0].colorSpace;
    swapchainCreateInfo.imageExtent = m_extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.preTransform = m_preTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = m_activePresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;

    err = vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapChain);
    if (err != VK_SUCCESS) {
        aout << "Error: Failed to create swapchain, error code: " << err << std::endl;
        return false;
    }

    aout << "Swapchain created successfully" << std::endl;

    if (!initializeImages()) {
        aout << "Error: Failed to create images" << std::endl;
        return false;
    }

    if (!initializeDepthBuffer()) {
        aout << "Error: Failed to create depth buffer" << std::endl;
        return false;
    }

    return true;
}

void Swapchain::destroy() {
    VkDevice device = RENDER_DEVICE->getDevice();

    vkDestroySwapchainKHR(device, m_swapChain, nullptr);

    /* Destroy swapchain depth buffer images */
    vkDestroyImageView(device, m_depthImage.imageView, nullptr);
    vkDestroyImage(device, m_depthImage.image, nullptr);
    vkFreeMemory(device, m_depthImage.memory, nullptr);

    for (auto view: m_imageViews) {
        vkDestroyImageView(device, view, nullptr);
    }
}

void Swapchain::resize(const uint32_t &width, const uint32_t &height) {
    VkDevice device = RENDER_DEVICE->getDevice();

    //vkDeviceWaitIdle(device);

    auto err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface,
                                                         &m_surfaceCapabilities);
    if (err != VK_SUCCESS) {
        aout << "Error: Failed to get surface capabilities" << std::endl;
        return;
    }

    auto transform = m_surfaceCapabilities.currentTransform;
    auto w = m_surfaceCapabilities.currentExtent.width;
    auto h = m_surfaceCapabilities.currentExtent.height;

    m_width = w;
    m_height = h;

    if (transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
        transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        std::swap(w, h);
    }

    VkSwapchainKHR oldSwapchain = m_swapChain;

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = m_surface;
    swapchainCreateInfo.minImageCount = m_imageCount;
    swapchainCreateInfo.imageFormat = m_imageFormat;
    swapchainCreateInfo.imageColorSpace = m_surfaceFormats[0].colorSpace;
    swapchainCreateInfo.imageExtent = {w, h};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.oldSwapchain = oldSwapchain;
    swapchainCreateInfo.preTransform = m_preTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = m_activePresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    swapchainCreateInfo.flags = 0;

    VkSwapchainKHR newSwapchain;

    auto result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &newSwapchain);
    if (result != VK_SUCCESS) {
        aout << "Error: Failed to create swapchain" << std::endl;
        return;
    }

    if (m_depthImage.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_depthImage.memory, nullptr);
    }

    if (m_depthImage.image != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_depthImage.image, nullptr);
    }

    if (m_depthImage.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_depthImage.imageView, nullptr);
    }

    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
    }

    m_swapChain = newSwapchain;

    if (!initializeImages()) {
        aout << "Error: Failed to create images" << std::endl;
        return;
    }

    if (!initializeDepthBuffer()) {
        aout << "Error: Failed to create depth buffer" << std::endl;
        return;
    }
}

bool Swapchain::initializeImages() {
    VkDevice device = RENDER_DEVICE->getDevice();

    uint32_t swapChainImageCount = 0;
    auto err = vkGetSwapchainImagesKHR(device, m_swapChain, &swapChainImageCount, nullptr);
    if (err != VK_SUCCESS) {
        aout << "Error: Failed to get swapchain images" << std::endl;
        return false;
    }

    m_images.resize(swapChainImageCount);
    err = vkGetSwapchainImagesKHR(device, m_swapChain, &swapChainImageCount, m_images.data());
    if (err != VK_SUCCESS) {
        aout << "Error: Failed to get swapchain images" << std::endl;
        return false;
    }

    /* Get Swapchain image views */
    for (auto view : m_imageViews) {
        vkDestroyImageView(device, view, nullptr);
    }
    m_imageViews.clear();

    m_imageViews.resize(swapChainImageCount);
    m_imageCount = swapChainImageCount;

    VkQueue queue = RENDER_DEVICE->getGraphicsQueue();

    for (auto i = 0; i < swapChainImageCount; i++) {

        VkCommandBuffer imageLayoutCmd;
        if (!RenderFramework::allocateCommandBuffer(RENDER_DEVICE->getPrimaryCommandPool(),
                                                    imageLayoutCmd)) {
            aout << "Error: Failed to allocate command buffer" << std::endl;
            return false;
        }

        VkCommandBufferInheritanceInfo inheritanceInfo{};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = VK_NULL_HANDLE;
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = VK_NULL_HANDLE;
        inheritanceInfo.occlusionQueryEnable = VK_FALSE;
        inheritanceInfo.queryFlags = 0;
        inheritanceInfo.pipelineStatistics = 0;
        inheritanceInfo.pNext = nullptr;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = &inheritanceInfo;
        beginInfo.pNext = nullptr;

        if (vkBeginCommandBuffer(imageLayoutCmd, &beginInfo) != VK_SUCCESS) {
            aout << "Error: Failed to begin command buffer" << std::endl;
            return false;
        }

        VkImageViewCreateInfo imgInfo{};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgInfo.image = m_images[i];
        imgInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgInfo.format = m_imageFormat;
        imgInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imgInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imgInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imgInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imgInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgInfo.subresourceRange.baseMipLevel = 0;
        imgInfo.subresourceRange.levelCount = 1;
        imgInfo.subresourceRange.baseArrayLayer = 0;
        imgInfo.subresourceRange.layerCount = 1;

        RenderFramework::setImageLayout(imageLayoutCmd, m_images[i], VK_IMAGE_ASPECT_COLOR_BIT,
                                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                        imgInfo.subresourceRange, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        vkEndCommandBuffer(imageLayoutCmd);

        /* Submit command buffer to queue */
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &imageLayoutCmd;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;

        if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            aout << "Error: Failed to submit command buffer" << std::endl;
            return false;
        }

        if (vkQueueWaitIdle(queue) != VK_SUCCESS) {
            aout << "Error: Failed to wait for queue idle" << std::endl;
            return false;
        }

        err = vkCreateImageView(device, &imgInfo, nullptr, &m_imageViews[i]);
        if (err != VK_SUCCESS) {
            aout << "Error: Failed to create image view" << std::endl;
            return false;
        }
    }

    m_currentImageIndex = 0;

    return true;
}

bool Swapchain::initializeDepthBuffer() {
    VkDevice device = RENDER_DEVICE->getDevice();
    VkPhysicalDevice physicalDevice = RENDER_DEVICE->getPhysicalDevice();
    VkQueue queue = RENDER_DEVICE->getGraphicsQueue();

    VkImageCreateInfo imageInfo{};
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_extent.width;
    imageInfo.extent.height = m_extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkFormatProperties formatProperties;

    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageInfo.format, &formatProperties);

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    } else if (formatProperties.linearTilingFeatures &
               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    } else {
        aout << "Error: Depth buffer format not supported" << std::endl;
        return false;
    }

    if (vkCreateImage(device, &imageInfo, nullptr, &m_depthImage.image) != VK_SUCCESS) {
        aout << "Error: Failed to create depth buffer image" << std::endl;
        return false;
    }

    m_depthImage.format = imageInfo.format;

    /* Allocate Depth Image Memory */
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_depthImage.image, &memRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = 0;

    if (!RenderFramework::findMemoryType(physicalDevice, memRequirements.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                         memoryAllocateInfo.memoryTypeIndex)) {
        aout << "Error: Failed to find memory type" << std::endl;
        return false;
    }

    if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &m_depthImage.memory) !=
        VK_SUCCESS) {
        aout << "Error: Failed to allocate memory" << std::endl;
        return false;
    }

    if (vkBindImageMemory(device, m_depthImage.image, m_depthImage.memory, 0) != VK_SUCCESS) {
        aout << "Error: Failed to bind memory" << std::endl;
        return false;
    }

    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = m_depthImage.image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageInfo.format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    if (m_depthImage.format == VK_FORMAT_D16_UNORM_S8_UINT ||
        m_depthImage.format == VK_FORMAT_D24_UNORM_S8_UINT ||
        m_depthImage.format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        imageViewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    imageViewCreateInfo.image = m_depthImage.image;

    VkCommandBuffer imageLayoutCmd;
    if (!RenderFramework::allocateCommandBuffer(RENDER_DEVICE->getPrimaryCommandPool(),
                                                imageLayoutCmd)) {
        aout << "Error: Failed to allocate command buffer" << std::endl;
        return false;
    }

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = VK_NULL_HANDLE;
    inheritanceInfo.subpass = 0;
    inheritanceInfo.framebuffer = VK_NULL_HANDLE;
    inheritanceInfo.occlusionQueryEnable = VK_FALSE;
    inheritanceInfo.queryFlags = 0;
    inheritanceInfo.pipelineStatistics = 0;
    inheritanceInfo.pNext = nullptr;
    inheritanceInfo.pipelineStatistics = 0;
    inheritanceInfo.pNext = nullptr;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.pNext = nullptr;

    if (vkBeginCommandBuffer(imageLayoutCmd, &beginInfo) != VK_SUCCESS) {
        aout << "Error: Failed to begin command buffer" << std::endl;
        return false;
    }

    RenderFramework::setImageLayout(imageLayoutCmd, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                    imageViewCreateInfo.subresourceRange,
                                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    vkEndCommandBuffer(imageLayoutCmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &imageLayoutCmd;

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        aout << "Error: Failed to submit command buffer" << std::endl;
        return false;
    }

    if (vkQueueWaitIdle(queue) != VK_SUCCESS) {
        aout << "Error: Failed to wait for queue idle" << std::endl;
        return false;
    }

    if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_depthImage.imageView) !=
        VK_SUCCESS) {
        aout << "Error: Failed to create image view" << std::endl;
        return false;
    }

    return true;
}
