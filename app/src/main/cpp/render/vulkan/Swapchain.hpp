//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#ifndef OXYOUS_2026_SWAPCHAIN_HPP
#define OXYOUS_2026_SWAPCHAIN_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"
#include "../../system/OGSingleton.hpp"

class Swapchain {
public:
    Swapchain();
    ~Swapchain();

    /* Initialize Render Swapchain */
    bool initialize(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t& width, uint32_t& height);

    /* Destroy Swapchain */
    void destroy();

    /* Resize Swapchain */
    void resize(const uint32_t& width, const uint32_t& height);

    /* */
    virtual VkFormat getImageFormat() { return m_imageFormat; }

    /* */
    virtual VkFormat getDepthFormat() { return m_depthImage.format; }

    /* */
    virtual VkImageView getDepthImageView() { return m_depthImage.imageView; }

    /* */
    virtual uint32_t getImageCount() { return m_imageCount; }

    /* */
    virtual VkExtent2D getExtent() { return m_extent; }

    /* */
    virtual VkImageView getImageView(uint32_t index) { return m_imageViews[index]; }

    /* */
    virtual VkSwapchainKHR getSwapChain() { return m_swapChain; }

protected:
    /* Initialize Images */
    bool initializeImages();

    /* Initialize Depth Buffer */
    bool initializeDepthBuffer();
protected:
    VkSwapchainKHR m_swapChain{VK_NULL_HANDLE};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    std::vector<VkPresentModeKHR> m_presentModes;
    VkPresentModeKHR m_activePresentMode{VK_PRESENT_MODE_FIFO_KHR};
    VkFormat m_imageFormat{VK_FORMAT_UNDEFINED};
    VkExtent2D m_extent{0, 0};
    GPUImage m_depthImage{};
private:
    std::vector<VkSurfaceFormatKHR> m_surfaceFormats;
    VkSurfaceTransformFlagBitsKHR m_preTransform;
    VkSurfaceKHR m_surface{};
    VkSurfaceCapabilitiesKHR m_surfaceCapabilities{};
    VkPhysicalDevice m_physicalDevice{};
    VkDevice m_device{};
    uint32_t m_imageCount{};
    uint32_t m_width{}, m_height{};
    uint32_t m_currentImageIndex{};
};

#define SWAPCHAIN OGSingleton<Swapchain>::getInstance()


#endif //OXYOUS_2026_SWAPCHAIN_HPP
