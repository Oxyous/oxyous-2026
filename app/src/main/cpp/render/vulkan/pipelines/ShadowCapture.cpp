//
// Created by Mr Steven J Baldwin on 29/06/2026.
//

#include "ShadowCapture.hpp"
#include "../../../AndroidOut.h"
#include "../RenderDevice.hpp"
#include "../RenderFramework.hpp"
#include "../../../engine/GPUResources.hpp"
#include "../../../engine/GameView.hpp"
#include "../../../engine/components/OGStaticMeshComponent.hpp"
#include "../RenderHelper.hpp"
#include "../../../engine/Engine.hpp"

ShadowCapture::ShadowCapture() {

}

ShadowCapture::~ShadowCapture() {

}

bool ShadowCapture::initialize() {

    if (!initializeRenderPass()) {
        aout << "Failed to initialize render pass!" << std::endl;
        return false;
    }

    /* Create Sampler */
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(RENDER_DEVICE->getDevice(), &samplerInfo, nullptr, &m_sampler) !=
        VK_SUCCESS) {
        aout << "Failed to create shadow sampler!" << std::endl;
        return false;
    }

    if (!initializeFrameBuffer()) {
        aout << "Failed to initialize frame buffer!" << std::endl;
        return false;
    }

    std::vector<uint8_t> vertexShaderCode;

    RESOURCE_MANAGER->loadShader("shaders/shadow.vert.spv", vertexShaderCode);

    if (vertexShaderCode.empty()) {
        aout << "Failed to load shader!" << std::endl;
        return false;
    }
    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo{};
    vertexShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleCreateInfo.codeSize = vertexShaderCode.size();
    vertexShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(vertexShaderCode.data());

    if (vkCreateShaderModule(RENDER_DEVICE->getDevice(), &vertexShaderModuleCreateInfo, nullptr,
                             &vertexShaderModule) != VK_SUCCESS) {
        aout << "Failed to create vertex shader module!" << std::endl;
        return false;
    }

    std::vector<uint8_t> fragShaderCode;
    RESOURCE_MANAGER->loadShader("shaders/shadow.frag.spv", fragShaderCode);

    VkShaderModuleCreateInfo fragShaderModuleCreateInfo{};
    fragShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderModuleCreateInfo.codeSize = fragShaderCode.size();
    fragShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(fragShaderCode.data());

    if (vkCreateShaderModule(RENDER_DEVICE->getDevice(), &fragShaderModuleCreateInfo, nullptr,
                             &fragShaderModule) != VK_SUCCESS) {
        aout << "Failed to create fragment shader module!" << std::endl;
        return false;
    }

    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo{};
    vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageCreateInfo.module = vertexShaderModule;
    vertexShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderCreateInfo{};
    fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderCreateInfo.module = fragShaderModule;
    fragShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo,
                                                      fragShaderCreateInfo};

    /* Initialize Descriptors */
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(RENDER_DEVICE->getDevice(), &layoutInfo, nullptr,
                                    &m_shadowDSL) != VK_SUCCESS) {
        aout << "Failed to create shadow descriptor set layout!" << std::endl;
        return false;
    }

    /* Create Pipeline Layout */
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ShadowMapPushConstants);

    VkDescriptorSetLayout setLayouts[] = {GPU_RESOURCES->getBindlessSetLayout(), m_shadowDSL};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(RENDER_DEVICE->getDevice(), &pipelineLayoutInfo, nullptr,
                               &m_pipelineLayout) != VK_SUCCESS) {
        aout << "Failed to create shadow pipeline layout!" << std::endl;
        return false;
    }

    /* Create Descriptor pool*/
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 2; // 1 * 2

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = &poolSize;
    descriptorPoolInfo.maxSets = 2; // 1 * 2

    if (vkCreateDescriptorPool(RENDER_DEVICE->getDevice(), &descriptorPoolInfo, nullptr,
                               &m_descriptorPool) != VK_SUCCESS) {
        aout << "Failed to create shadow descriptor pool!" << std::endl;
        return false;
    }

    for (int i = 0; i < 2; i++) {
        /* Allocate Descriptor sets*/
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_shadowDSL;

        if (vkAllocateDescriptorSets(RENDER_DEVICE->getDevice(), &allocInfo, &m_shadowSets[i]) !=
            VK_SUCCESS) {
            aout << "Failed to allocate shadow descriptor sets!" << std::endl;
            return false;
        }
    }

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
    viewport.width = (float) m_shadowMapSize;
    viewport.height = (float) m_shadowMapSize;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_shadowMapSize, m_shadowMapSize};

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
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    /* Dynamic State */
    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // 2. Create a dummy color blend state
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 0;
    colorBlending.pAttachments = nullptr;


    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult res = vkCreateGraphicsPipelines(RENDER_DEVICE->getDevice(), VK_NULL_HANDLE, 1,
                                             &pipelineInfo,
                                             nullptr, &m_pipeline);

    /* Step 6 Destroy Shader Modules */
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), vertexShaderModule, nullptr);
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), fragShaderModule, nullptr);

    if (res != VK_SUCCESS) {
        aout << "Failed to create graphics pipeline! Error code: " << res << std::endl;
        m_pipeline = VK_NULL_HANDLE;
        return false;
    }

    m_uniformBuffer.initialize<CSMUBO>({
                                               .lightProjection = {
                                                       glm::mat4(1.0f),
                                                       glm::mat4(1.0f),
                                                       glm::mat4(1.0f),
                                                       glm::mat4(1.0f)
                                               },
                                               .cascadeSplits = glm::vec4(1.0f)
                                       });

    for (int i = 0; i < 2; i++) {
        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_shadowSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = m_uniformBuffer.getDescriptorInfo(i);

        vkUpdateDescriptorSets(RENDER_DEVICE->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    return true;
}

void ShadowCapture::destroy() {
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

    for (auto framebuffer: frameBuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    frameBuffers.clear();

    for (auto imageView: imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    imageViews.clear();

    if (m_shadowMapImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_shadowMapImageView, nullptr);
        m_shadowMapImageView = VK_NULL_HANDLE;
    }

    if (m_shadowMapImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_shadowMapImage, nullptr);
        m_shadowMapImage = VK_NULL_HANDLE;
    }

    if (m_shadowMapMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_shadowMapMemory, nullptr);
        m_shadowMapMemory = VK_NULL_HANDLE;
    }

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
}

void ShadowCapture::update(double delta) {
    CSMData gpuData = ENGINE->getSharedCSMData();
    m_uniformBuffer.update(&gpuData, ENGINE->getCurrentFrame());
}

void ShadowCapture::render(VkCommandBuffer cmd, uint32_t currentFrame) {

}

bool ShadowCapture::initializeRenderPass() {
    std::array<VkAttachmentDescription, 1> attachments = {};
    attachments[0].format = VK_FORMAT_D32_SFLOAT;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(RENDER_DEVICE->getDevice(), &renderPassInfo, nullptr, &m_renderPass) !=
        VK_SUCCESS) {
        aout << "Failed to create render pass!" << std::endl;
        return false;
    }

    return true;
}

bool ShadowCapture::initializeFrameBuffer() {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_shadowMapSize;
    imageInfo.extent.height = m_shadowMapSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = m_cascadeCount;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    if (vkCreateImage(RENDER_DEVICE->getDevice(), &imageInfo, nullptr, &m_shadowMapImage) !=
        VK_SUCCESS) {
        aout << "Failed to create shadow map image!" << std::endl;
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(RENDER_DEVICE->getDevice(), m_shadowMapImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;

    uint32_t memoryTypeIndex = 0;
    if (!RenderFramework::findMemoryType(RENDER_DEVICE->getPhysicalDevice(),
                                         memRequirements.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeIndex)) {
        aout << "Failed to find suitable memory type!" << std::endl;
        return false;
    }

    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(RENDER_DEVICE->getDevice(), &allocInfo, nullptr, &m_shadowMapMemory) !=
        VK_SUCCESS) {
        aout << "Failed to allocate shadow map memory!" << std::endl;
        return false;
    }

    if (vkBindImageMemory(RENDER_DEVICE->getDevice(), m_shadowMapImage, m_shadowMapMemory, 0) !=
        VK_SUCCESS) {
        aout << "Failed to bind shadow map memory!" << std::endl;
        return false;
    }

    /* Create shadow map view for the whole image (all cascades) */
    VkImageViewCreateInfo shadowMapViewInfo{};
    shadowMapViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    shadowMapViewInfo.image = m_shadowMapImage;
    shadowMapViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    shadowMapViewInfo.format = VK_FORMAT_D32_SFLOAT;
    shadowMapViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    shadowMapViewInfo.subresourceRange.baseMipLevel = 0;
    shadowMapViewInfo.subresourceRange.levelCount = 1;
    shadowMapViewInfo.subresourceRange.baseArrayLayer = 0;
    shadowMapViewInfo.subresourceRange.layerCount = m_cascadeCount;

    if (vkCreateImageView(RENDER_DEVICE->getDevice(), &shadowMapViewInfo, nullptr,
                          &m_shadowMapImageView) != VK_SUCCESS) {
        aout << "Failed to create shadow map image view!" << std::endl;
        return false;
    }

    m_shadowMapDescriptor.sampler = m_sampler;
    m_shadowMapDescriptor.imageView = m_shadowMapImageView;
    m_shadowMapDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    imageViews.resize(m_cascadeCount);
    frameBuffers.resize(m_cascadeCount);

    for (int i = 0; i < m_cascadeCount; ++i) {
        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = m_shadowMapImage;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = VK_FORMAT_D32_SFLOAT;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = i;
        imageViewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(RENDER_DEVICE->getDevice(), &imageViewInfo, nullptr,
                              &imageViews[i]) != VK_SUCCESS) {
            aout << "Failed to create shadow map image view!" << std::endl;
            return false;
        }
    }

    for (int i = 0; i < m_cascadeCount; ++i) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &imageViews[i];
        framebufferInfo.width = m_shadowMapSize;
        framebufferInfo.height = m_shadowMapSize;
        framebufferInfo.layers = 1;
        framebufferInfo.renderPass = m_renderPass;

        if (vkCreateFramebuffer(RENDER_DEVICE->getDevice(), &framebufferInfo, nullptr,
                                &frameBuffers[i]) != VK_SUCCESS) {
            aout << "Failed to create shadow map framebuffer!" << std::endl;
            return false;
        }
    }

    return true;
}

void ShadowCapture::bindPipeline(VkCommandBuffer const &commandBuffer) {
    if (m_pipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    uint32_t currentFrame = ENGINE->getCurrentFrame();

    VkDescriptorSet setsToBind[] = {GPU_RESOURCES->getBindlessSet(currentFrame),
                                    m_shadowSets[currentFrame % 2]};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 2,
                            setsToBind, 0, nullptr);
}


void ShadowCapture::record(VkCommandBuffer commandBuffer, uint64_t currentFrame,
                           VkFramebuffer framebuffer) {
    if (m_pipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    /* Bind Descriptor Sets */
    VkDescriptorSet setsToBind[] = {GPU_RESOURCES->getBindlessSet(currentFrame),
                                    m_shadowSets[currentFrame % 2]};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 2,
                            setsToBind, 0, nullptr);

    // Fetch consolidated data
    CSMData gpuData = ENGINE->getSharedCSMData();

    std::vector<OGEntity*> dynamicObjects;
    GAME_VIEW->getDynamicObjects(dynamicObjects);

    // Compute Cascade Splits
    auto splits = RenderHelper::computeCascadeSplits(0.1f, 1000.0f, 0.98f);

    for (int i = 0; i < m_cascadeCount; ++i) {
        AABBVolume cascadeVolume = RenderHelper::computeCSMBounds(ENGINE->getCameraProjection(),
                                                                  ENGINE->getCameraView(),
                                                                  glm::vec3(0.5, 1.0, 0.5),
                                                                  splits[i], splits[i + 1]);
        std::vector<AABBVolume> visibleVolumes;
        std::vector<OGEntity*> visibleObjects;
        ENGINE->queryOctree(cascadeVolume, visibleVolumes);

        visibleObjects.reserve(visibleVolumes.size());
        for (const auto& vol: visibleVolumes){
            visibleObjects.push_back(vol.getOwner());
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = frameBuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {m_shadowMapSize, m_shadowMapSize};

        VkClearValue clearValues[1];
        clearValues[0].depthStencil.depth = 1.0f;
        clearValues[0].depthStencil.stencil = 0;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) m_shadowMapSize;
        viewport.height = (float) m_shadowMapSize;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {m_shadowMapSize, m_shadowMapSize};

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        /* Render Objects */
        for (auto &obj: visibleObjects) {
            const auto entity = obj;
            if (!entity) {
                continue;
            }
            for (auto child: entity->getChildren()) {
                if (child->getComponent<OGStaticMeshComponent>()) {
                    auto *component = child->getComponent<OGStaticMeshComponent>();
                    if (component) {
                        component->renderShadow(commandBuffer, currentFrame, m_pipelineLayout,
                                                gpuData, i);
                    }
                }
            }

            if (entity->getComponent<OGStaticMeshComponent>()) {
                auto *component = entity->getComponent<OGStaticMeshComponent>();
                if (component) {
                    component->renderShadow(commandBuffer, currentFrame, m_pipelineLayout, gpuData,
                                            i);
                }
            }
        }

        for (const auto & obj: dynamicObjects) {
            if (obj->getComponent<OGStaticMeshComponent>()) {
                auto *component = obj->getComponent<OGStaticMeshComponent>();
                if (component) {
                    component->renderShadow(commandBuffer, currentFrame, m_pipelineLayout, gpuData,
                                            i);
                }
            }
        }

        vkCmdEndRenderPass(commandBuffer);
    }
}

void ShadowCapture::execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                            const VkFence &fence) {

}

void ShadowCapture::resize(int width, int height) {

}

VkDescriptorImageInfo ShadowCapture::getFrameBufferImage(const std::string &name) {
    if (name == "gShadow") {
        return m_shadowMapDescriptor;
    }
    return VkDescriptorImageInfo();
}
