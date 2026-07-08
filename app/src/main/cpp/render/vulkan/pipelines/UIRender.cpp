//
// Created by Mr Steven J Baldwin on 08/07/2026.
//

#include "UIRender.hpp"
#include "AndroidOut.h"
#include "resources/ResourceManager.hpp"
#include "render/vulkan/RenderDevice.hpp"
#include "engine/elements/ScreenSpaceRenderer.hpp"
#include "engine/ui/OGUi.hpp"
#include "engine/Engine.hpp"
#include "render/vulkan/Swapchain.hpp"
#include "render/vulkan/RenderFramework.hpp"

bool UIRender::initialize() {

    if (m_renderPass == VK_NULL_HANDLE && !initializeRenderPass()) {
        aout << "Failed to initialize render pass";
        return false;
    }

    std::vector<uint8_t> vertShaderCode;
    std::vector<uint8_t> fragShaderCode;

    RESOURCE_MANAGER->loadShader("shaders/ui.vert.spv", vertShaderCode);
    RESOURCE_MANAGER->loadShader("shaders/ui.frag.spv", fragShaderCode);

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        aout << "Failed to load UI shaders";
        return false;
    }

    /** Create Vertex Shader Module*/

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
    std::array<VkVertexInputBindingDescription, 2> bindingDescription{};
    bindingDescription[0].binding = 0;
    bindingDescription[0].stride = sizeof(OGVertex2D);
    bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindingDescription[1].binding = 1;
    bindingDescription[1].stride = sizeof(SpriteInstance);
    bindingDescription[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(OGVertex2D, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(OGVertex2D, uv);

    /** Sprite Instance bindings*/
    attributeDescriptions[2].binding = 1;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(SpriteInstance, position);

    attributeDescriptions[3].binding = 1;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(SpriteInstance, size);

    attributeDescriptions[4].binding = 1;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(SpriteInstance, uvOffset);

    attributeDescriptions[5].binding = 1;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[5].offset = offsetof(SpriteInstance, uvScale);

    /** Create Pipeline info */

    VkPipelineVertexInputStateCreateInfo vertexInput{};

    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
    vertexInput.pVertexBindingDescriptions = bindingDescription.data();

    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // Dynamic state
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr; // Dynamic state

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

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.blendEnable = VK_TRUE;

    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

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

    /** Initialize Descriptor Sets*/
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    VkDescriptorSetLayout descriptorSetLayout;

    vkCreateDescriptorSetLayout(
            RENDER_DEVICE->getDevice(),
            &layoutInfo,
            nullptr,
            &descriptorSetLayout);


    /** Create Pipeline Push Constant */
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    /** Create Pipeline Layout*/
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    vkCreatePipelineLayout(
            RENDER_DEVICE->getDevice(),
            &pipelineLayoutInfo,
            nullptr,
            &m_pipelineLayout);

    /** */
    if (!initializeDescriptorPool())  {
        aout << "Failed to initialize descriptor pool" << std::endl;
        return false;
    }

    /** Allocate Descriptor */
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    vkAllocateDescriptorSets(
            RENDER_DEVICE->getDevice(),
            &allocInfo,
            &m_descriptorSet);

    /** Create Graphics Pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
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

    //?
    vkDestroyDescriptorSetLayout(RENDER_DEVICE->getDevice(), descriptorSetLayout, nullptr);

    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_vertShaderModule, nullptr);
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_fragShaderModule, nullptr);

    /* Initialize Quad Vertex Buffer*/

    if(!RenderFramework::createStagingBuffer(spriteVertices, sizeof(spriteVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &m_vertexBuffer)) {
        aout << "Failed to create quad vertex buffer!" << std::endl;
        return false;
    }

    if(!RenderFramework::createStagingBuffer(spriteIndices, sizeof(spriteIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &m_indexBuffer)) {
        aout << "Failed to create quad index buffer!" << std::endl;
        return false;
    }

    if(!RenderFramework::createBuffer(sizeof(SpriteInstance) * MAX_SPRITES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,nullptr, &m_instanceBuffer)) {
        aout << "Failed to create instance buffer!" << std::endl;
        return false;
    }
    return true;
}

void UIRender::record(VkCommandBuffer cmd, uint64_t currentFrame, VkFramebuffer framebuffer) {

    float swapWidth = static_cast<float>(SWAPCHAIN->getExtent().width);
    float swapHeight = static_cast<float>(SWAPCHAIN->getExtent().height);
    float designWidth = (swapWidth * DESIGN_HEIGHT) / swapHeight;
    auto ortho = ENGINE->preRotation() * glm::ortho(0.0f, designWidth, 0.0f, DESIGN_HEIGHT, -1.0f, 1.0f);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = SWAPCHAIN->getExtent();


    renderPassInfo.clearValueCount = 0;
    renderPassInfo.pClearValues = nullptr;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapWidth;
    viewport.height = swapHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = SWAPCHAIN->getExtent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkBuffer vertexBuffers[] =
            {
                    m_vertexBuffer.buffer,
                    m_instanceBuffer.buffer
            };

    VkDeviceSize offsets[] =
            {
                    0,
                    0
            };

    vkCmdBindVertexBuffers(
            cmd,
            0,
            2,
            vertexBuffers,
            offsets);

    vkCmdBindIndexBuffer(
            cmd,
            m_indexBuffer.buffer,
            0,
            VK_INDEX_TYPE_UINT16);

    /* Bind the bindless descriptor set Once*/
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 1,
                            &m_descriptorSet, 0, nullptr);


    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &ortho);

    auto& instances = UI->getInstances();
    if (!instances.empty()) {
        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(sizeof(spriteIndices) / sizeof(uint16_t)), static_cast<uint32_t>(instances.size()), 0, 0, 0);
    }



    vkCmdEndRenderPass(cmd);

}

void UIRender::setRenderPass(VkRenderPass renderPass) {
    IRenderPipeline::setRenderPass(renderPass);
    m_renderPass = renderPass;
}

bool UIRender::initializeRenderPass() {
    std::array<VkAttachmentDescription, 2> attachments = {};
    attachments[0].format = SWAPCHAIN->getImageFormat();
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].format = SWAPCHAIN->getDepthFormat();
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 2> dependencies = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = 0;
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

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(
            RENDER_DEVICE->getDevice(),
            &renderPassInfo,
            nullptr,
            &m_renderPass) != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

bool UIRender::initializeDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(
            RENDER_DEVICE->getDevice(),
            &poolInfo,
            nullptr,
            &m_descriptorPool) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void UIRender::updateInstances() {
    auto& instances = UI->getInstances();
    if (!instances.empty()) {
        const VkDeviceSize uploadSize = sizeof(SpriteInstance) * instances.size();
        if (uploadSize > m_instanceBuffer.size) {
            aout << "UIRender::updateInstances overflow: " << uploadSize
                 << " > " << m_instanceBuffer.size << std::endl;
            return;
        }
        memcpy(m_instanceBuffer.mapped, instances.data(), static_cast<size_t>(uploadSize));
    }
}

void UIRender::update(double delta) {
    updateInstances();
}

void UIRender::execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                       const VkFence &fence) {

}

void UIRender::destroy() {

}

void UIRender::resize(int width, int height) {

}

void UIRender::bindPipeline(VkCommandBuffer const &commandBuffer) {

}

void UIRender::updateDescriptorSet() {
    if (m_descriptorSet == VK_NULL_HANDLE) {
        aout << "UIRender::updateDescriptorSet descriptor set is null" << std::endl;
        return;
    }

    auto texture = UI->getAtlasTexture();
    if (texture == nullptr) {
        aout << "UIRender::updateDescriptorSet atlas texture is null" << std::endl;
        return;
    }
    if (texture->image.imageView == VK_NULL_HANDLE || texture->sampler == VK_NULL_HANDLE) {
        aout << "UIRender::updateDescriptorSet atlas texture handles are invalid" << std::endl;
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->image.imageView;
    imageInfo.sampler = texture->sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(RENDER_DEVICE->getDevice(),
                           1,
                           &descriptorWrite,
                           0,
                           nullptr);
}