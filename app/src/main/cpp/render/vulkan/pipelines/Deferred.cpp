//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#include "Deferred.hpp"
#include "../RenderDevice.hpp"
#include "../Swapchain.hpp"
#include "../../../resources/ResourceManager.hpp"
#include "../../../engine/components/OGStaticMeshComponent.hpp"
#include "../../../engine/Engine.hpp"
#include "../../../engine/GameView.hpp"
#include "../../../engine/GPUResources.hpp"

Deferred::Deferred() : m_width(0), m_height(0), m_vertShaderModule(VK_NULL_HANDLE),
                       m_fragShaderModule(VK_NULL_HANDLE) {

}


void Deferred::update(double delta) {

}

void Deferred::execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                       const VkFence &fence) {

}

bool Deferred::initialize() {
    m_width = SWAPCHAIN->getExtent().width;
    m_height = SWAPCHAIN->getExtent().height;

    if (!initializeRenderPass()) {
        aout << "Failed to initialize render pass!" << std::endl;
        return false;
    }

    if (!initializeFramebuffers()) {
        aout << "Failed to initialize framebuffers!" << std::endl;
        return false;
    }

    /* Create Sampler */
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    if (vkCreateSampler(RENDER_DEVICE->getDevice(), &samplerInfo, nullptr, &m_sampler) !=
        VK_SUCCESS) {
        aout << "Failed to create sampler!" << std::endl;
        return false;
    }

    /* Step 2 - Shaders */
    std::vector<uint8_t> vertShaderCode;
    std::vector<uint8_t> fragShaderCode;

    RESOURCE_MANAGER->loadShader("shaders/bindless-deferred.vert.spv", vertShaderCode);
    RESOURCE_MANAGER->loadShader("shaders/bindless-deferred.frag.spv", fragShaderCode);

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        aout << "Failed to load shaders!" << std::endl;
        return false;
    }

    VkShaderModuleCreateInfo vertShaderModuleInfo{};
    vertShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderModuleInfo.codeSize = vertShaderCode.size();
    vertShaderModuleInfo.pCode = reinterpret_cast<const uint32_t *>(vertShaderCode.data());
    if (vkCreateShaderModule(RENDER_DEVICE->getDevice(), &vertShaderModuleInfo, nullptr,
                             &m_vertShaderModule) != VK_SUCCESS) {
        aout << "Failed to create vertex shader module!" << std::endl;
        return false;
    }

    VkShaderModuleCreateInfo fragShaderModuleInfo{};
    fragShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderModuleInfo.codeSize = fragShaderCode.size();
    fragShaderModuleInfo.pCode = reinterpret_cast<const uint32_t *>(fragShaderCode.data());
    if (vkCreateShaderModule(RENDER_DEVICE->getDevice(), &fragShaderModuleInfo, nullptr,
                             &m_fragShaderModule) != VK_SUCCESS) {
        aout << "Failed to create fragment shader module!" << std::endl;
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    m_pipelineLayout = GPU_RESOURCES->getPipelineLayout();

    /* Step 3 Fixed Function states */
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(StaticMeshVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(StaticMeshVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(StaticMeshVertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(StaticMeshVertex, tangent);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(StaticMeshVertex, uv);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    /* Input assembly */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    /* Viewport and Scissor */
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_width;
    viewport.height = (float) m_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_width, m_height};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    /* Rasterizer */
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.alphaToCoverageEnable = VK_TRUE;

    /* Depth and Stencil State */
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachments[4] = {};
    for (int i = 0; i < 4; ++i) {
        colorBlendAttachments[i].colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachments[i].blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 4;
    colorBlending.pAttachments = colorBlendAttachments;

    /* Dynamic State */
    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    /* Step 5 Graphics Pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo{};
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

    if (vkCreateGraphicsPipelines(RENDER_DEVICE->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr,
                                  &m_pipeline) != VK_SUCCESS) {
        aout << "Failed to create graphics pipeline!" << std::endl;
        return false;
    }

    /* Step 6 Destroy Shader Modules */
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_vertShaderModule, nullptr);
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_fragShaderModule, nullptr);

    return true;
}

void Deferred::destroy() {
    VkDevice device = RENDER_DEVICE->getDevice();

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_frameBuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, m_frameBuffer, nullptr);
        m_frameBuffer = VK_NULL_HANDLE;
    }

    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }

    for (auto &pair: m_frameBufferImages) {
        //  RenderFramework::destroyGPUTexture(&pair.second);
    }
    m_frameBufferImages.clear();
}

void Deferred::resize(int width, int height) {
    m_width = width;
    m_height = height;

    VkDevice device = RENDER_DEVICE->getDevice();
    vkDeviceWaitIdle(device);

    // Destroy old resources
    if (m_frameBuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, m_frameBuffer, nullptr);
        m_frameBuffer = VK_NULL_HANDLE;
    }

    for (auto &pair: m_frameBufferImages) {
        vkDestroyImageView(device, pair.second.image.imageView, nullptr);
        vkDestroyImage(device, pair.second.image.image, nullptr);
        vkFreeMemory(device, pair.second.image.memory, nullptr);
    }
    m_frameBufferImages.clear();

    // Re-initialize
    initializeFramebuffers();
}

void Deferred::bindPipeline(VkCommandBuffer const &commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

VkDescriptorImageInfo *Deferred::getFrameBufferImage(const std::string &name) {
    if (m_frameBufferImages.find(name) == m_frameBufferImages.end()) {
        aout << "Frame buffer image not found!" << std::endl;
        return nullptr;
    }

    VkDescriptorImageInfo *descriptor = &m_frameBufferImages[name].descriptor;
    if (descriptor->sampler == VK_NULL_HANDLE) {
        descriptor->sampler = m_sampler;
    }

    if (descriptor->imageView == VK_NULL_HANDLE) {
        descriptor->imageView = m_frameBufferImages[name].image.imageView;
    }

    if (descriptor->imageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        descriptor->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    return descriptor;
}

bool Deferred::initializeRenderPass() {
    std::array<VkAttachmentDescription, 5> attachments = {};
    // Diffuse
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Normal
    attachments[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // PBR
    attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // World Position
    attachments[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Depth
    attachments[4].format = VK_FORMAT_D16_UNORM;
    attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    std::array<VkAttachmentReference, 4> colorReferences = {};
    for (int i = 0; i < 4; ++i) {
        colorReferences[i].attachment = i;
        colorReferences[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 4;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pColorAttachments = colorReferences.data();
    subpass.pDepthStencilAttachment = &depthReference;

    std::array<VkSubpassDependency, 2> dependencies = {};
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

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(RENDER_DEVICE->getDevice(), &renderPassInfo, nullptr, &m_renderPass) !=
        VK_SUCCESS) {
        return false;
    }

    return true;
}

bool Deferred::initializeFramebuffers() {

    if (!RenderFramework::createFrameBufferImage(VK_FORMAT_R8G8B8A8_UNORM,
                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                                                 m_width,
                                                 m_height,
                                                 &m_frameBufferImages["gDiffuse"])) {
        aout << "Failed to create diffuse framebuffer image!" << std::endl;
        return false;
    }

    if (!RenderFramework::createFrameBufferImage(VK_FORMAT_R32G32B32A32_SFLOAT,
                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                                                 m_width,
                                                 m_height,
                                                 &m_frameBufferImages["gNormal"])) {
        aout << "Failed to create normal framebuffer image!" << std::endl;
        return false;
    }

    if (!RenderFramework::createFrameBufferImage(VK_FORMAT_R8G8B8A8_UNORM,
                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                                                 m_width,
                                                 m_height,
                                                 &m_frameBufferImages["gPBR"])) {
        aout << "Failed to create gPBR framebuffer image!" << std::endl;
        return false;
    }

    if (!RenderFramework::createFrameBufferImage(VK_FORMAT_R32G32B32A32_SFLOAT,
                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                                                 m_width,
                                                 m_height,
                                                 &m_frameBufferImages["gWorldPosition"])) {
        aout << "Failed to create gWorldPosition framebuffer image!" << std::endl;
        return false;
    }

    if (!RenderFramework::createFrameBufferImage(VK_FORMAT_D16_UNORM,
                                                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                 VK_IMAGE_USAGE_SAMPLED_BIT, m_width, m_height,
                                                 &m_frameBufferImages["gDepth"])) {
        aout << "Failed to create depth framebuffer image!" << std::endl;
        return false;
    }

    /*  */
    VkImageView attachments[5] = {
            m_frameBufferImages["gDiffuse"].image.imageView,
            m_frameBufferImages["gNormal"].image.imageView,
            m_frameBufferImages["gPBR"].image.imageView,
            m_frameBufferImages["gWorldPosition"].image.imageView,
            m_frameBufferImages["gDepth"].image.imageView
    };

    /* Create the frame buffer*/
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = 5;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = m_width;
    framebufferInfo.height = m_height;
    framebufferInfo.layers = 1;
    framebufferInfo.flags = 0;

    if (vkCreateFramebuffer(RENDER_DEVICE->getDevice(), &framebufferInfo, nullptr,
                            &m_frameBuffer) != VK_SUCCESS) {
        aout << "Failed to create framebuffer!" << std::endl;
        return false;
    }

    return true;
}


void
Deferred::record(VkCommandBuffer commandBuffer, uint64_t currentFrame, VkFramebuffer framebuffer) {

    /* Upload Frame Data*/
    FrameData &frame = GPU_RESOURCES->getFrameData(currentFrame);

    const float aspect = static_cast<float>(m_height) / static_cast<float>(m_width);

    frame.perFrame.projection = ENGINE->getCameraProjection();
    frame.perFrame.view = ENGINE->getCameraView();


    GPU_RESOURCES->uploadFrameData(frame);

    /* Clear Framebuffers */
    VkClearValue clearValues[5] = {};
    clearValues[0].color = {{0.05f, 0.2f, 0.8f, 1.0f}}; // Diffuse
    clearValues[1].color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // Normal
    clearValues[2].color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // PBR
    clearValues[3].color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // World Position
    clearValues[4].depthStencil = {1.0f, 0}; // Depth

    /* Create Render Pass info */
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_frameBuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {m_width, m_height};
    renderPassInfo.clearValueCount = 5;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    /* Set Viewport */
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_width;
    viewport.height = (float) m_height;

    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    /* Set Scissor */
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {m_width, m_height};

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    /* Bind the bindless descriptor set Once*/
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 1,
                            &GPU_RESOURCES->getBindlessSet(currentFrame), 0, nullptr);

    /* Render Objects */
    auto &staticMeshes = GAME_VIEW->getEntities();
    for (auto &mesh: staticMeshes) {

        for (auto& child: mesh.second->getChildren()) {
            if (child->getComponent<OGStaticMeshComponent>()) {
                auto *component = child->getComponent<OGStaticMeshComponent>();
                if (component) {
                    component->render(commandBuffer, currentFrame);
                }
            }
        }

        if (mesh.second->getComponent<OGStaticMeshComponent>()) {
            auto *component = mesh.second->getComponent<OGStaticMeshComponent>();
            if (component) {
                component->render(commandBuffer, currentFrame);
            }
        }
    }

    vkCmdEndRenderPass(commandBuffer);
}
