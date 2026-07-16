//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#include "ScreenSpace.hpp"
#include "../RenderDevice.hpp"
#include "../../../resources/ResourceManager.hpp"
#include "../Swapchain.hpp"
#include "../../../engine/elements/ScreenSpaceRenderer.hpp"
#include "../../../engine/ui/OGUi.hpp"
#include "system/OGTimer.hpp"
#include "engine/Engine.hpp"
#include "render/vulkan/Renderer.hpp"
#include <iomanip>



void ScreenSpace::update(double delta) {

}

bool ScreenSpace::initialize() {

    if (!initializeRenderPass()) {
        aout << "Failed to initialize render pass!" << std::endl;
        return false;
    }

    std::vector<uint8_t> vertShaderCode;
    std::vector<uint8_t> fragShaderCode;

    RESOURCE_MANAGER->loadShader("shaders/screen-space.vert.spv", vertShaderCode);
    RESOURCE_MANAGER->loadShader("shaders/screen-space.frag.spv", fragShaderCode);

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        aout << "Failed to load shaders!" << std::endl;
        return false;
    }

    /* Create Vertex Shader Modules */
    VkShaderModuleCreateInfo vertShaderModuleInfo = {};
    vertShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderModuleInfo.codeSize = vertShaderCode.size();
    vertShaderModuleInfo.pCode = reinterpret_cast<const uint32_t *>(vertShaderCode.data());

    if (vkCreateShaderModule(RENDER_DEVICE->getDevice(), &vertShaderModuleInfo, nullptr,
                             &m_vertShaderModule) != VK_SUCCESS) {
        aout << "Failed to create vertex shader module!" << std::endl;
        return false;
    }

    /* Create Fragment Shader Modules */
    VkShaderModuleCreateInfo fragShaderModuleInfo = {};
    fragShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderModuleInfo.codeSize = fragShaderCode.size();
    fragShaderModuleInfo.pCode = reinterpret_cast<const uint32_t *>(fragShaderCode.data());

    if (vkCreateShaderModule(RENDER_DEVICE->getDevice(), &fragShaderModuleInfo, nullptr,
                             &m_fragShaderModule) != VK_SUCCESS) {
        aout << "Failed to create fragment shader module!" << std::endl;
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    /* Step 3 Fixed Function states */
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(OGVertex2D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(OGVertex2D, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(OGVertex2D, uv);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;

    viewport.width = (float) SWAPCHAIN->getExtent().width;
    viewport.height = (float) SWAPCHAIN->getExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = SWAPCHAIN->getExtent();

    /* Viewport state*/
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    /* */
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachments[2] = {};
    for (int i = 0; i < 2; ++i) {
        colorBlendAttachments[i].colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;

        colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].blendEnable = VK_TRUE;
    }

    colorBlending.attachmentCount = 2;
    colorBlending.pAttachments = colorBlendAttachments;

    /* Depth stencil state */
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;

    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    depthStencil.stencilTestEnable = VK_FALSE;

    /* Dynamic states */
    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    /* Initialize Descriptors*/
    m_pipelineLayout = SCREEN_RENDER->getPipelineLayout();

    /* Create Graphics Pipeline */
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
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult res = vkCreateGraphicsPipelines(RENDER_DEVICE->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    if (res != VK_SUCCESS) {
        aout << "Failed to create graphics pipeline! Error code: " << res << std::endl;
        return false;
    }

    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_vertShaderModule, nullptr);
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_fragShaderModule, nullptr);

    return true;
}

void ScreenSpace::execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                          const VkFence &fence) {

}

void ScreenSpace::destroy() {
    VkDevice device = RENDER_DEVICE->getDevice();

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
}

void ScreenSpace::resize(int width, int height) {

}

void ScreenSpace::record(VkCommandBuffer commandBuffer, uint64_t currentFrame,
                         VkFramebuffer framebuffer) {
    auto& frame = SCREEN_RENDER->getScreenElements(currentFrame);

    float width = (float) SWAPCHAIN->getExtent().width;
    float height = (float) SWAPCHAIN->getExtent().height;

    // Fixed design resolution for consistency across different screen sizes
    float aspect = width / height;
    float designWidth = DESIGN_HEIGHT * aspect;

    // Standard UI coordinate system: (0,0) at Top-Left, (designWidth, DESIGN_HEIGHT) at Bottom-Right
    // GLM's ortho(left, right, bottom, top) maps [bottom, top] to NDC [-1, 1].
    // In Vulkan, NDC Y is -1 at top and 1 at bottom.
    // So we map 0 to -1 and DESIGN_HEIGHT to 1.
    frame.perFrame.projection = ENGINE->preRotation() * glm::ortho(0.0f, width,  0.0f,height, -1.0f, 1.0f);

    frame.perFrame.screenSize = glm::vec2(designWidth, DESIGN_HEIGHT);

    SCREEN_RENDER->uploadFrameData(frame);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = SWAPCHAIN->getExtent();

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &SCREEN_RENDER->getBindLessSet(currentFrame), 0, nullptr);



    auto &elements = UI->getElements();
    for (auto &element : elements) {
        if (element && element->isVisible()) {
            element->draw(commandBuffer, static_cast<uint32_t>(currentFrame));
        }
    }

    std::stringstream ss;
    ss << "FPS : " << SYS_TIMER->getFPS();
    UI->drawString(commandBuffer, ss.str(), 32.0f,64.0f, 1.0f);

    ss.str("");
    ss.clear();
    if (ENGINE->getRenderer()) {
        ss << "GPU : " << std::fixed << std::setprecision(2) << ENGINE->getRenderer()->getGpuTime() << " ms";
        UI->drawString(commandBuffer, ss.str(), 32.0f, 128.0f, 1.0f);
    }

    ss.str("");
    ss.clear();

    JNIEnv* env = ENGINE->getJniEnv();
    jclass debugClass =
            env->FindClass("android/os/Debug");

    jmethodID mid =
            env->GetStaticMethodID(
                    debugClass,
                    "getNativeHeapAllocatedSize",
                    "()J");

    jlong bytes =
            env->CallStaticLongMethod(
                    debugClass,
                    mid);

    env->DeleteLocalRef(debugClass);

    ss << "Memory: " << (bytes / (1024 * 1024)) << " MB";
    UI->drawString(commandBuffer, ss.str(), 32.0f, 192.0f, 1.0f);
    ss.str("");
    ss.clear();

    ss << "Visible Count:" << ENGINE->getCachedVisibleObjects().size();
    UI->drawString(commandBuffer, ss.str(), 32.0f, 256.0f, 1.0f);

    ss.str("");
    ss.clear();

    ss << "Temperature:" << SYS_TIMER->getTemperature();
    UI->drawString(commandBuffer, ss.str(), 32.0f, 320.0f, 1.0f);

    ss.str("");
    ss.clear();

    vkCmdEndRenderPass(commandBuffer);
}

void ScreenSpace::setRenderPass(VkRenderPass renderPass) {

}

void ScreenSpace::bindPipeline(VkCommandBuffer const &commandBuffer) {

}

VkPipelineLayout ScreenSpace::getPipelineLayout() const {
    return IRenderPipeline::getPipelineLayout();
}

bool ScreenSpace::initializeRenderPass() {
    std::array<VkAttachmentDescription, 2> attachments = {};
    attachments[0].format = SWAPCHAIN->getImageFormat();
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].format = SWAPCHAIN->getDepthFormat();
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 2> dependency = {};
    dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency[0].dstSubpass = 0;
    dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency[0].srcAccessMask = 0;
    dependency[0].dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependency[1].srcSubpass = 0;
    dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency[1].srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependency.size());
    renderPassInfo.pDependencies = dependency.data();

    if (vkCreateRenderPass(RENDER_DEVICE->getDevice(), &renderPassInfo, nullptr, &m_renderPass) !=
        VK_SUCCESS) {
        aout << "Failed to create render pass!" << std::endl;
        return false;
    }

    return true;

}