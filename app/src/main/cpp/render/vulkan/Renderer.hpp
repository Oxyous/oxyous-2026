//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#ifndef OXYOUS_2026_RENDERER_HPP
#define OXYOUS_2026_RENDERER_HPP


#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>
#include "../../includes.hpp"
#include "../../DataStructures.hpp"
#include "Swapchain.hpp"
#include "pipelines/Deferred.hpp"

class Renderer {
public:
    virtual bool initialize(ANativeWindow* window);

    virtual void destroy();

    virtual void render();

    virtual void update(double delta);

    virtual void setWidth(uint32_t width) { m_width = width; }

    virtual void setHeight(uint32_t height) { m_height = height; }

    virtual VkRenderPass getRenderPass() const { return m_renderPass; }

    virtual ~Renderer() = default;
protected:

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanCallback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType,
                                                         uint64_t srcObj, size_t location, int32_t msgCode, const char *layerPrefix,
                                                         const char *msg, void *userData);
    /* Initialize Semaphores */
    bool initializeSemaphores();

    /* Initialize Fences */
    bool initializeFences();

    /* Initialize Render Pass */
    bool initializeRenderPass();

    /* Initialize Framebuffers */
    bool initializeFramebuffers();

    /* Prepare Command Buffers */
    bool prepareCommandBuffers();

    /* Recreate Swap Chain */
    void recreateSwapChain();
protected:
    void prepareFrame(int index, VkCommandBuffer commandBuffer);
protected:
    uint32_t m_width{};
    uint32_t m_height{};

    VkDebugReportCallbackCreateInfoEXT m_debugReportCallbackCreateInfo{};
    VkDebugReportCallbackEXT m_debugReportCallback{};
    VkRenderPass m_renderPass{};
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkSemaphore> m_presentCompleteSemaphores;
    std::vector<VkSemaphore> m_renderCompleteSemaphores;
    std::vector<VkFence> m_fences;
    std::vector<VkCommandBuffer> m_commandBuffers;

    appEngine* m_engine{};
    VkInstance m_instance{};
    VkSurfaceKHR m_surface{};

    ANativeWindow* m_window{};

    bool m_graphicsInitialized{false};
    uint32_t m_currentFrame{0};
    const int MAX_FRAMES_IN_FLIGHT = 2;
};


#endif //OXYOUS_2026_RENDERER_HPP
