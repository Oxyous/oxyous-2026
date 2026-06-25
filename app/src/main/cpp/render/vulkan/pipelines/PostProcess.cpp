//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "PostProcess.hpp"
#include "../RenderDevice.hpp"
#include "../../../resources/ResourceManager.hpp"
#include "../Swapchain.hpp"

PostProcess::PostProcess() = default;

PostProcess::~PostProcess() {
    destroy();
}

void PostProcess::resize(int width, int height) {
    m_width = width;
    m_height = height;
}

void PostProcess::update(double delta) {
    /*m_uniformBuffer.update<PostProcessUBO>({
            .projection = glm::ortho(0.0f, (float) m_width, (float) m_height, 0.0f, -1.0f, 1.0f),
            .view = glm::mat4(1.0f),
            .invView = glm::mat4(1.0f),
            .cameraPosition = glm::vec4(8.0f, 8.0f, 8.0f, 1.0f)
    });*/
}

void PostProcess::execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                          const VkFence &fence) {

}

bool PostProcess::initializeRenderPass() {
    const auto &device = RENDER_DEVICE->getDevice();

    /* Create Render Pass */
    std::array<VkAttachmentDescription, 2> attachments = {};
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    attachments[1].format = VK_FORMAT_D16_UNORM;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /* Create Color Attachment Reference */
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    /* Create Depth Attachment Reference */
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /* Create Subpass*/
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    /* Use subpass for dependencies */
    std::array<VkSubpassDependency, 2> dependency = {};
    dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency[0].dstSubpass = 0;
    dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency[0].srcAccessMask = 0;
    dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency[0].dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependency[1].srcSubpass = 0;
    dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency[1].srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    /* Create Render Pass*/

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependency.size());
    renderPassInfo.pDependencies = dependency.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        aout << "Failed to create render pass!" << std::endl;
        return false;
    }

    return true;
}

bool PostProcess::initialize() {
    // Cleanup if already initialized
    destroy();

    m_width = SWAPCHAIN->getExtent().width;
    m_height = SWAPCHAIN->getExtent().height;

    // If m_renderPass is still null, we initialize our own,
    // but typically the Renderer will set it to the swapchain render pass.
    if (m_renderPass == VK_NULL_HANDLE) {
        if (!initializeRenderPass()) {
            aout << "Failed to initialize render pass!" << std::endl;
            return false;
        }
    }

    /* Get Vertex Shader Code */
    std::vector<uint8_t> vertShaderCode;
    std::vector<uint8_t> fragShaderCode;

    RESOURCE_MANAGER->loadShader("shaders/postprocess.vert.spv", vertShaderCode);
    RESOURCE_MANAGER->loadShader("shaders/postprocess.frag.spv", fragShaderCode);

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

    /* Vertexless drawing - no input bindings or attributes */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    /* Pipeline Vertex input assembly */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    /* Pipeline Viewport and scissor */
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_width;
    viewport.height = (float) m_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {m_width, m_height};

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    /* Rasterizer pipeline settings */
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    /* Multisample pipeline settings */
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    /* Dynamic states */
    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Depth stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;

    /* Initialize Descriptors*/
    if (!initializeDescriptors()) {
        aout << "Failed to initialize descriptors!" << std::endl;
        return false;
    }

    /* Pipeline Descriptor Layout */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_perFrameDSL;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(RENDER_DEVICE->getDevice(), &pipelineLayoutInfo, nullptr,
                               &m_pipelineLayout) != VK_SUCCESS) {
        aout << "Failed to create pipeline layout!" << std::endl;
        return false;
    }

    /* Create Descriptor pool*/
    VkDescriptorPoolSize poolSize[2] = {};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[0].descriptorCount = 4;
    poolSize[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = 2;
    descriptorPoolInfo.pPoolSizes = poolSize;
    descriptorPoolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(RENDER_DEVICE->getDevice(), &descriptorPoolInfo, nullptr,
                               &m_descriptorPool) != VK_SUCCESS) {
        aout << "Failed to create descriptor pool!" << std::endl;
        return false;
    }

    /* Allocate Descriptor sets*/
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_perFrameDSL;

    if (vkAllocateDescriptorSets(RENDER_DEVICE->getDevice(), &allocInfo, &m_descriptorSet) !=
        VK_SUCCESS) {
        aout << "Failed to allocate descriptor sets!" << std::endl;
        return false;
    }

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

    if (vkCreateGraphicsPipelines(RENDER_DEVICE->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &m_pipeline) != VK_SUCCESS) {
        aout << "Failed to create graphics pipeline!" << std::endl;
        return false;
    }

    /* Step 6 Destroy Shader Modules */
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_vertShaderModule, nullptr);
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_fragShaderModule, nullptr);

    /* Step 7 - Create Uniform Buffer*/
    if (!m_uniformBuffer.initialize<PostProcessUBO>({
        .projection = glm::ortho(0.0f, (float) m_width, (float) m_height, 0.0f, -1.0f, 1.0f),
        .view = glm::mat4(1.0f),
        .invView = glm::mat4(1.0f),
        .cameraPosition = glm::vec4(8.0f, 8.0f, 8.0f, 1.0f)
    })){
        aout << "Failed to create uniform buffer!" << std::endl;
        return false;
    }

    return true;
}

bool PostProcess::initializeDescriptors() {
    const auto &device = RENDER_DEVICE->getDevice();
    VkDescriptorSetLayoutBinding layoutBinding[5] = {};

    /* Diffuse GBuffer */
    layoutBinding[0].binding = 0;
    layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding[0].descriptorCount = 1;
    layoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    /* Normal Depth Buffer*/
    layoutBinding[1].binding = 1;
    layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding[1].descriptorCount = 1;
    layoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    /* PBR Buffer */
    layoutBinding[2].binding = 2;
    layoutBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding[2].descriptorCount = 1;
    layoutBinding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    /* World Position Buffer */
    layoutBinding[3].binding = 3;
    layoutBinding[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding[3].descriptorCount = 1;
    layoutBinding[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    /* Uniform Buffer */
    layoutBinding[4].binding = 4;
    layoutBinding[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding[4].descriptorCount = 1;
    layoutBinding[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 5;
    layoutInfo.pBindings = layoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_perFrameDSL) != VK_SUCCESS) {
        aout << "Failed to create descriptor set layout!" << std::endl;
        return false;
    }

    return true;
}

void PostProcess::bindPipeline(VkCommandBuffer const &commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                            &m_descriptorSet, 0, nullptr);
}


void PostProcess::destroy() {
    VkDevice device = RENDER_DEVICE->getDevice();

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_perFrameDSL != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_perFrameDSL, nullptr);
        m_perFrameDSL = VK_NULL_HANDLE;
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
}

void PostProcess::setFrameBufferImage(const std::string &name, const VkDescriptorImageInfo &image) {
    m_frameBufferImages[name] = image;
}

void PostProcess::updateDescriptors() {
    const auto &device = RENDER_DEVICE->getDevice();
    VkWriteDescriptorSet descriptorWrites[5] = {};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &m_frameBufferImages["gDiffuse"];

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &m_frameBufferImages["gNormal"];

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = m_descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo = &m_frameBufferImages["gPBR"];

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = m_descriptorSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pImageInfo = &m_frameBufferImages["gWorldPosition"];

    descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[4].dstSet = m_descriptorSet;
    descriptorWrites[4].dstBinding = 4;
    descriptorWrites[4].dstArrayElement = 0;
    descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[4].descriptorCount = 1;
    descriptorWrites[4].pBufferInfo = m_uniformBuffer.getDescriptorInfo();

    vkUpdateDescriptorSets(device, 5, descriptorWrites, 0, nullptr);
}

void PostProcess::record(VkCommandBuffer commandBuffer, uint64_t currentFrame, VkFramebuffer framebuffer) {
    if (framebuffer == VK_NULL_HANDLE) {
        aout << "PostProcess::record called with null framebuffer!" << std::endl;
        return;
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {m_width, m_height};

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

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

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                            &m_descriptorSet, 0, nullptr);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}
