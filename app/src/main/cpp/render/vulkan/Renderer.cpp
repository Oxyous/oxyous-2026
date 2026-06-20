//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#include "Renderer.hpp"
#include "../../AndroidOut.h"
#include "RenderDevice.hpp"
#include "../../DataStructures.hpp"
#include "../../engine/GameView.hpp"
#include <game-activity/native_app_glue/android_native_app_glue.h>

VkBool32
Renderer::vulkanCallback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType,
                         uint64_t srcObj, size_t location, int32_t msgCode, const char *layerPrefix,
                         const char *msg, void *userData) {

    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        aout << "Vulkan Error: " << msg << std::endl;
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        aout << "Vulkan Warning: " << msg << std::endl;
    } else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        aout << "Vulkan Performance Warning: " << msg << std::endl;
    } else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        aout << "Vulkan Info: " << msg << std::endl;
    } else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        aout << "Vulkan Debug: " << msg << std::endl;
    }

    return VK_TRUE;
}

bool Renderer::initialize(ANativeWindow* window) {
    //if (m_graphicsInitialized)
    //    return true;

    m_window = window;

    /* Get Instance Layer Properties */
    uint32_t instanceLayerCount = 0;
    if (vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr) != VK_SUCCESS) {
        aout << "Error: Failed to enumerate instance layer properties" << std::endl;
        return false;
    }

    std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
    if (vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data()) !=
        VK_SUCCESS) {
        aout << "Error: Failed to enumerate instance layer properties" << std::endl;
        return false;
    }

    for (const auto &layer: instanceLayers) {
        aout << "Instance Layer: " << layer.layerName << std::endl;
    }

    /* Get Instance Extension Properties */
    uint32_t instanceExtensionCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr) !=
        VK_SUCCESS) {
        aout << "Error: Failed to enumerate instance extension properties" << std::endl;
        return false;
    }

    /* */
    uint32_t extensionCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
        aout << "Error: Failed to enumerate instance extension properties" << std::endl;
        return false;
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()) !=
        VK_SUCCESS) {
        aout << "Error: Failed to enumerate instance extension properties" << std::endl;
        return false;
    }

    for (const auto &extension: extensions) {
        aout << "Instance Extension: " << extension.extensionName << std::endl;
    }

    std::vector<const char *> enabledLayers;
    std::vector<const char *> enabledExtensions;

    enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
    bool debugReportExtensionPresent = false;
    for (const auto &extension: extensions) {
        if (strcmp(extension.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            debugReportExtensionPresent = true;
            break;
        }
    }

    if (debugReportExtensionPresent) {
        enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        aout << "Enabling instance extension: " << VK_EXT_DEBUG_REPORT_EXTENSION_NAME << std::endl;
    }

    /* */
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Oxyous 2026";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Oxyous 2026 Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    createInfo.ppEnabledLayerNames = enabledLayers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        aout << "Error: Failed to create Vulkan instance, error code: " << result << std::endl;
        return false;
    }

    if (debugReportExtensionPresent) {
        if (RENDER_DEVICE->initializeDebug(m_instance)) {
            m_debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
            m_debugReportCallbackCreateInfo.flags =
                    VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            m_debugReportCallbackCreateInfo.pfnCallback = vulkanCallback;
            m_debugReportCallbackCreateInfo.pUserData = nullptr;

            result = RenderDevice::fvkCreateDebugReportCallbackEXT(m_instance,
                                                                   &m_debugReportCallbackCreateInfo,
                                                                   nullptr, &m_debugReportCallback);
            if (result != VK_SUCCESS) {
                aout << "Warning: Failed to create Vulkan debug report callback" << std::endl;
            }
        } else {
            aout << "Warning: Failed to initialize Vulkan debug functions" << std::endl;
        }
    }

    /* Android Surface Create */
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = window;
    surfaceCreateInfo.flags = 0;

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

    m_deferredPipeline.setRenderPass(m_renderPass);

    if(!m_deferredPipeline.initialize()){
        aout << "Error: Failed to initialize deferred pipeline" << std::endl;
        return false;
    }

    if (!prepareCommandBuffers()){
        aout << "Error: Failed to prepare command buffers" << std::endl;
        return false;
    }

    if(!GAME_VIEW->initialize()){
        aout << "Error: Failed to initialize game view" << std::endl;
        return false;
    }

    m_graphicsInitialized = true;

    return true;
}

bool Renderer::initializeSemaphores() {

    VkDevice device = RENDER_DEVICE->getDevice();
    uint32_t imageCount = SWAPCHAIN->getImageCount();

    m_presentCompleteSemaphores.resize(imageCount);
    m_renderCompleteSemaphores.resize(imageCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (uint32_t i = 0; i < imageCount; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_presentCompleteSemaphores[i]) != VK_SUCCESS) {
            aout << "Error: Failed to create present complete semaphore" << std::endl;
            return false;
        }

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderCompleteSemaphores[i]) != VK_SUCCESS) {
            aout << "Error: Failed to create render complete semaphore" << std::endl;
            return false;
        }
    }

    return true;
}

bool Renderer::initializeFences() {
    VkDevice device = RENDER_DEVICE->getDevice();
    
    m_fences.resize(SWAPCHAIN->getImageCount());
    
    for (auto& fence : m_fences) {
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        if (vkCreateFence(device, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
            aout << "Error: Failed to create fence" << std::endl;
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

    for (auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    m_framebuffers.clear();

    if (!m_commandBuffers.empty()) {
        vkFreeCommandBuffers(device, RENDER_DEVICE->getPrimaryCommandPool(),
                             static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
        m_commandBuffers.clear();
    }

    m_deferredPipeline.destroy();

    vkDestroyRenderPass(device, m_renderPass, nullptr);

    for (auto semaphore : m_presentCompleteSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    m_presentCompleteSemaphores.clear();

    for (auto semaphore : m_renderCompleteSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    m_renderCompleteSemaphores.clear();

    for (auto fence : m_fences) {
        vkDestroyFence(device, fence, nullptr);
    }
    m_fences.clear();

    SWAPCHAIN->destroy();

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;

    RENDER_DEVICE->destroy();

    if (m_debugReportCallback) {
        RenderDevice::fvkDestroyDebugReportCallbackEXT(m_instance, m_debugReportCallback, nullptr);
    }

    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
    m_graphicsInitialized = false;
}

void Renderer::recreateSwapChain() {
    VkDevice device = RENDER_DEVICE->getDevice();
    vkDeviceWaitIdle(device);

    // Destroy old fences
    for (auto fence : m_fences) {
        vkDestroyFence(device, fence, nullptr);
    }
    m_fences.clear();

    // Destroy old semaphores
    for (auto semaphore : m_presentCompleteSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    m_presentCompleteSemaphores.clear();

    for (auto semaphore : m_renderCompleteSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    m_renderCompleteSemaphores.clear();

    // Swapchain resize handles internal recreation
    SWAPCHAIN->resize(m_width, m_height);

    uint32_t imageCount = SWAPCHAIN->getImageCount();

    // Recreate semaphores for the new swapchain image count
    m_presentCompleteSemaphores.resize(imageCount);
    m_renderCompleteSemaphores.resize(imageCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (uint32_t i = 0; i < imageCount; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_presentCompleteSemaphores[i]) != VK_SUCCESS) {
            aout << "Error: Failed to create present complete semaphore during swapchain recreation" << std::endl;
        }

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderCompleteSemaphores[i]) != VK_SUCCESS) {
            aout << "Error: Failed to create render complete semaphore during swapchain recreation" << std::endl;
        }
    }

    // Recreate fences for the new swapchain image count
    m_fences.resize(imageCount);
    for (auto& fence : m_fences) {
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        if (vkCreateFence(device, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
            aout << "Error: Failed to create fence during swapchain recreation" << std::endl;
        }
    }

    // Re-initialize resources that depend on the swapchain
    if (!initializeFramebuffers()) {
        aout << "Error: Failed to re-initialize framebuffers during swapchain recreation" << std::endl;
    }

    if (!prepareCommandBuffers()) {
        aout << "Error: Failed to re-prepare command buffers during swapchain recreation" << std::endl;
    }
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
    VkClearValue clearValues[2];
    clearValues[0].color.float32[0] = 0.15f;
    clearValues[0].color.float32[1] = 0.32f;
    clearValues[0].color.float32[2] = 0.65f;
    clearValues[0].color.float32[3] = 1.0f;
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[index];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = SWAPCHAIN->getExtent();
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) SWAPCHAIN->getExtent().width;
    viewport.height = (float) SWAPCHAIN->getExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = SWAPCHAIN->getExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    m_deferredPipeline.bindPipeline(commandBuffer);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        aout << "Error: Failed to end command buffer" << std::endl;
        return;
    }
}
