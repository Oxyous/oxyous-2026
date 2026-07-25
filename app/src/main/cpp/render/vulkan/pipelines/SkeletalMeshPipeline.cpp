//
// Created by Mr Steven J Baldwin on 25/07/2026.
//

#include <stdint.h>
#include "SkeletalMeshPipeline.hpp"
#include "resources/ResourceManager.hpp"
#include "render/vulkan/RenderDevice.hpp"
#include "engine/GPUResources.hpp"
#include "engine/Engine.hpp"

void SkeletalMeshPipeline::update(double delta) {

}

bool SkeletalMeshPipeline::initialize() {

    /** Shaders */
    std::vector<uint8_t> vertShaderCode;
    std::vector<uint8_t> fragShaderCode;

    RESOURCE_MANAGER->loadShader("shaders/skeletal-deferred.vert.spv", vertShaderCode);
    RESOURCE_MANAGER->loadShader("shaders/skeletal-deferred.frag.spv", fragShaderCode);

    VkShaderModuleCreateInfo vertShaderModuleInfo{};
    vertShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderModuleInfo.codeSize = vertShaderCode.size();
    vertShaderModuleInfo.pCode = reinterpret_cast<const uint32_t *>(vertShaderCode.data());
    if (vkCreateShaderModule(RENDER_DEVICE->getDevice(), &vertShaderModuleInfo, nullptr,
                             &m_vertexShaderModule) != VK_SUCCESS) {
        aout << "Failed to create vertex shader module!" << std::endl;
        return false;
    }

    VkShaderModuleCreateInfo fragShaderModuleInfo{};
    fragShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderModuleInfo.codeSize = fragShaderCode.size();
    fragShaderModuleInfo.pCode = reinterpret_cast<const uint32_t *>(fragShaderCode.data());
    if (vkCreateShaderModule(RENDER_DEVICE->getDevice(), &fragShaderModuleInfo, nullptr,
                             &m_fragmentShaderModule) != VK_SUCCESS) {
        aout << "Failed to create fragment shader module!" << std::endl;
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertexShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_fragmentShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    /** Pipeline Layout */
    m_pipelineLayout = GPU_RESOURCES->getPipelineLayout();

    /* Step 3 Fixed Function states */
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(OGSkeletalVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(OGSkeletalVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(OGSkeletalVertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(OGSkeletalVertex, tangent);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(OGSkeletalVertex, uv);

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(OGSkeletalVertex, boneWeights);

    attributeDescriptions[5].binding = 0;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SINT;
    attributeDescriptions[5].offset = offsetof(OGSkeletalVertex, boneIndices);

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

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = m_parentPipeline->getViewportState();
    pipelineInfo.pRasterizationState = m_parentPipeline->getRasterizationState();
    pipelineInfo.pMultisampleState = m_parentPipeline->getMultisampleState();
    pipelineInfo.pDepthStencilState = m_parentPipeline->getDepthStencilState();
    pipelineInfo.pColorBlendState = m_parentPipeline->getColorBlendState();
    pipelineInfo.pDynamicState = m_parentPipeline->getDynamicState();
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_parentPipeline->getRenderPass();
    pipelineInfo.subpass = 0;

    auto res = vkCreateGraphicsPipelines(RENDER_DEVICE->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    if (res != VK_SUCCESS) {
        aout << "Failed to create graphics pipeline!" << std::endl;
        return false;
    }

    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_vertexShaderModule, nullptr);
    vkDestroyShaderModule(RENDER_DEVICE->getDevice(), m_fragmentShaderModule, nullptr);

    return true;
}

void
SkeletalMeshPipeline::execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                              const VkFence &fence) {

}

void SkeletalMeshPipeline::destroy() {

}

void SkeletalMeshPipeline::resize(int width, int height) {

}

void SkeletalMeshPipeline::record(VkCommandBuffer commandBuffer, uint64_t currentFrame,
                                  VkFramebuffer framebuffer) {

}

void SkeletalMeshPipeline::setRenderPass(VkRenderPass renderPass) {
    IRenderPipeline::setRenderPass(renderPass);
}

void SkeletalMeshPipeline::bindPipeline(VkCommandBuffer const &commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    uint32_t currentFrame = ENGINE->getCurrentFrame();
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 1, 1,
                            &GPU_RESOURCES->getBoneSet(currentFrame), 0, nullptr);
}

VkPipelineLayout SkeletalMeshPipeline::getPipelineLayout() const {
    return IRenderPipeline::getPipelineLayout();
}
