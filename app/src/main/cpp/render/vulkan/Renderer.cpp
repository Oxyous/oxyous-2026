//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#include "Renderer.hpp"
#include "../../AndroidOut.h"
#include "RenderDevice.hpp"
#include "../../DataStructures.hpp"
#include "../../engine/GameView.hpp"
#include "../../engine/Engine.hpp"
#include "pipelines/PostProcess.hpp"
#include "../../engine/GPUResources.hpp"
#include <game-activity/native_app_glue/android_native_app_glue.h>

VkBool32
Renderer::vulkanCallback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType,
                         uint64_t srcObj, size_t location, int32_t msgCode, const char *layerPrefix,
                         const char *msg, void *userData) {
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        aout << "ERROR: " << layerPrefix << " - " << msg << std::endl;
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        aout << "WARNING: " << layerPrefix << " - " << msg << std::endl;
    } else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        aout << "PERF: " << layerPrefix << " - " << msg << std::endl;
    } else {
        aout << "INFO: " << layerPrefix << " - " << msg << std::endl;
    }

    return VK_FALSE;
}

bool Renderer::initialize(ANativeWindow *window) {
    m_window = window;
    m_width = ANativeWindow_getWidth(window);
    m_height = ANativeWindow_getHeight(window);

    VkResult result;

    /* Create Vulkan Instance */
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Oxyous 2026";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Oxyous Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 3);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const char *extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 3;
    createInfo.ppEnabledExtensionNames = extensions;

    result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        aout << "Error: Failed to create Vulkan instance, error code: " << result << std::endl;
        return false;
    }

    /* Create Debug Report Callback */
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
    debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugReportCallbackCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debugReportCallbackCreateInfo.pfnCallback = vulkanCallback;

    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(
            m_instance, "vkCreateDebugReportCallbackEXT");
    if (vkCreateDebugReportCallbackEXT != nullptr) {
        vkCreateDebugReportCallbackEXT(m_instance, &debugReportCallbackCreateInfo, nullptr,
                                       &m_debugReportCallback);
    }

    /* Create Android Surface */
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = window;

    result = vkCreateAndroidSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface);
    if (result != VK_SUCCESS) {
        aout << "Error: Failed to create Vulkan Android surface, error code: " << result << std::endl;
        return false;
    }

    if (RENDER_DEVICE->initialize(m_instance, m_surface) == false) {
        aout << "Error: Failed to initialize RenderDevice" << std::endl;
        return false;
    }

    if (!SWAPCHAIN->initialize(m_surface, RENDER_DEVICE->getPhysicalDevice(),
                                RENDER_DEVICE->getDevice(), m_width, m_height)) {
        aout << "Error: Failed to initialize Vulkan swap chain" << std::endl;
        return false;
    }

    if (!initializeSemaphores()) {
        aout << "Error: Failed to initialize Vulkan semaphores" << std::endl;
        return false;
    }

    if (!initializeRenderPass()) {
        aout << "Error: Failed to initialize Vulkan render pass" << std::endl;
        return false;
    }

    if (!initializeFences()) {
        aout << "Error: Failed to initialize Vulkan fences" << std::endl;
        return false;
    }

    if (!initializeFramebuffers()) {
        aout << "Error: Failed to initialize Vulkan framebuffers" << std::endl;
        return false;
    }

    if (!prepareCommandBuffers()){
        aout << "Error: Failed to prepare command buffers" << std::endl;
        return false;
    }

    if (!GPU_RESOURCES->initialize()) {
        aout << "Error: Failed to initialize GPU resources" << std::endl;
        return false;
    }

    if(!GAME_VIEW->initialize()){
        aout << "Error: Failed to initialize game view" << std::endl;
        return false;
    }

    auto postProcess = ENGINE->getPipeline<PostProcess>("post-process");
    if (postProcess) {
        postProcess->setRenderPass(m_renderPass);
        postProcess->initialize();
        postProcess->updateDescriptors();
    }


    m_graphicsInitialized = true;

    return true;
}

bool Renderer::initializeSemaphores() {
    VkDevice device = RENDER_DEVICE->getDevice();
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    m_presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_presentCompleteSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderCompleteSemaphores[i]) !=
            VK_SUCCESS) {
            aout << "Error: Failed to create semaphores" << std::endl;
            return false;
        }
    }

    return true;
}

bool Renderer::initializeFences() {
    VkDevice device = RENDER_DEVICE->getDevice();
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    m_fences.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(device, &fenceInfo, nullptr, &m_fences[i]) != VK_SUCCESS) {
            aout << "Error: Failed to create fences" << std::endl;
            return false;
        }
    }

    return true;
}

bool Renderer::initializeRenderPass() {
    VkAttachmentDescription attachmentDescription[2];   // Color and Depth attachments

    attachmentDescription[0].format = SWAPCHAIN->getImageFormat();
    attachmentDescription[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachmentDescription[1].format = SWAPCHAIN->getDepthFormat();
    attachmentDescription[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /* Create Attachment References */
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    /* Depth Attachment Reference */
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /* Create sub pass */
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    /* Subpass dependencies */
    VkSubpassDependency dependencies[2];
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    /* Create Render Pass */
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachmentDescription;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    if (vkCreateRenderPass(RENDER_DEVICE->getDevice(), &renderPassInfo, nullptr, &m_renderPass) !=
        VK_SUCCESS) {
        aout << "Error: Failed to create render pass" << std::endl;
        return false;
    }

    return true;
}

bool Renderer::initializeFramebuffers() {
    VkDevice device = RENDER_DEVICE->getDevice();

    /* Cleanup existing framebuffers if any */
    for (auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    m_framebuffers.clear();

    VkImageView attachments[2];
    attachments[1] = SWAPCHAIN->getDepthImageView();

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = SWAPCHAIN->getExtent().width;
    framebufferInfo.height = SWAPCHAIN->getExtent().height;
    framebufferInfo.layers = 1;

    const uint32_t imageCount = SWAPCHAIN->getImageCount();

    m_framebuffers.clear();
    m_framebuffers.resize(imageCount);

    /* Create Framebuffers for each swap chain image */
    for (uint32_t i = 0; i < imageCount; ++i) {
        attachments[0] = SWAPCHAIN->getImageView(i);
        framebufferInfo.pAttachments = attachments;

        if (vkCreateFramebuffer(RENDER_DEVICE->getDevice(), &framebufferInfo, nullptr,
                                &m_framebuffers[i]) != VK_SUCCESS) {
            aout << "Error: Failed to create framebuffer" << std::endl;
            return false;
        }
    }

    return true;
}

void Renderer::destroy() {
    VkDevice device = RENDER_DEVICE->getDevice();

    vkDeviceWaitIdle(device);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, m_presentCompleteSemaphores[i], nullptr);
        vkDestroySemaphore(device, m_renderCompleteSemaphores[i], nullptr);
        vkDestroyFence(device, m_fences[i], nullptr);
    }

    for (auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyRenderPass(device, m_renderPass, nullptr);

    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugReportCallbackEXT");
    if (vkDestroyDebugReportCallbackEXT != nullptr) {
        vkDestroyDebugReportCallbackEXT(m_instance, m_debugReportCallback, nullptr);
    }

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void Renderer::recreateSwapChain() {

}

void Renderer::render() {
    if (!m_graphicsInitialized) return;
    VkDevice device = RENDER_DEVICE->getDevice();
    VkQueue graphicsQueue = RENDER_DEVICE->getGraphicsQueue();

    // Wait for the current frame's fence to ensure the GPU has finished using the resources (semaphores, cmd buffers) of this frame slot
    if (vkWaitForFences(device, 1, &m_fences[m_currentFrame], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        aout << "Error: Failed to wait for fence" << std::endl;
        return;
    }

    vkResetFences(device, 1, &m_fences[m_currentFrame]);

    uint32_t imageIndex = 0;
    // Acquire signals m_presentCompleteSemaphores[m_currentFrame] when image is ready
    VkResult result = vkAcquireNextImageKHR(device, SWAPCHAIN->getSwapChain(), UINT64_MAX,
                                           m_presentCompleteSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_TIMEOUT) {
        return;
    } else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_SURFACE_LOST_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        aout << "Error: Failed to acquire next image, result: " << result << std::endl;
        return;
    }

    // Reset the fence before re-using it
    if (vkResetFences(device, 1, &m_fences[m_currentFrame]) != VK_SUCCESS) {
        aout << "Error: Failed to reset fence" << std::endl;
        return;
    }

    // Record command buffer for this frame
    VkCommandBuffer commandBuffer = m_commandBuffers[imageIndex];
    
    if (vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT) != VK_SUCCESS) {
        aout << "Error: Failed to reset command buffer" << std::endl;
        return;
    }
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        aout << "Error: Failed to begin command buffer" << std::endl;
        return;
    }

    prepareFrame(imageIndex, commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        aout << "Error: Failed to end command buffer" << std::endl;
        return;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_presentCompleteSemaphores[m_currentFrame];

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_renderCompleteSemaphores[m_currentFrame];

    auto res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_fences[m_currentFrame]);

    if (res == VK_ERROR_DEVICE_LOST) {
        aout << "Error: Device Lost during submit" << std::endl;
        return;
    }
    if (res != VK_SUCCESS) {
        aout << "Error: Failed to submit command buffer. Result: " << res << std::endl;
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderCompleteSemaphores[m_currentFrame];
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapChains[] = {SWAPCHAIN->getSwapChain()};
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(RENDER_DEVICE->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_SURFACE_LOST_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        aout << "Error: Failed to present image" << std::endl;
        return;
    }

    // Advance to next frame slot
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool Renderer::prepareCommandBuffers() {
    VkDevice device = RENDER_DEVICE->getDevice();

    /* Cleanup existing command buffers if any */
    if (!m_commandBuffers.empty()) {
        vkFreeCommandBuffers(device, RENDER_DEVICE->getPrimaryCommandPool(),
                             static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
    }

    m_commandBuffers.clear();
    m_commandBuffers.resize(SWAPCHAIN->getImageCount());

    for (uint32_t i = 0; i < m_commandBuffers.size(); i++) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = RENDER_DEVICE->getPrimaryCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &allocInfo, &m_commandBuffers[i]) != VK_SUCCESS) {
            aout << "Error: Failed to allocate command buffer" << std::endl;
            return false;
        }
    }

    return true;
}

/*  */
void Renderer::prepareFrame(int index, VkCommandBuffer commandBuffer) {
    auto deferred = ENGINE->getPipeline<Deferred>("deferred");
    auto postProcess = ENGINE->getPipeline<PostProcess>("post-process");

    if (deferred) {
        deferred->record(commandBuffer, m_currentFrame, m_framebuffers[index]);
    }

    if (postProcess) {
        postProcess->record(commandBuffer, m_currentFrame, m_framebuffers[index]);
    }
}

void Renderer::update(double delta) {
    if (!m_graphicsInitialized) return;
    GAME_VIEW->update(delta);
}
