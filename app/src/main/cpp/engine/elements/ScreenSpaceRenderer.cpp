//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#include "ScreenSpaceRenderer.hpp"
#include "../../render/vulkan/RenderDevice.hpp"
#include "../../render/vulkan/RenderFramework.hpp"

bool ScreenSpaceRenderer::initialize() {

    if (!createBindlessDescriptors()) {
        aout << "Failed to create bindless descriptors!" << std::endl;
        return false;
    }

    if (!createPipelineLayout()) {
        aout << "Failed to create pipeline layout!" << std::endl;
        return false;
    }

    if (!initializeFrames()) {
        aout << "Failed to initialize frames!" << std::endl;
        return false;
    }

    return true;
}

void ScreenSpaceRenderer::uploadFrameData(ScreenElements &frameData) {
    if (!m_bindlessRenderer.elements.empty()) {
        std::memcpy(frameData.meshBuffer.mapped, m_bindlessRenderer.elements.data(),
                    sizeof(GPUElementHandle) * m_bindlessRenderer.elements.size());
    }

    std::memcpy(frameData.perFrameBuffer.mapped, &frameData.perFrame, sizeof(ScreenSpaceUBO));
}

uint32_t ScreenSpaceRenderer::registerTexture(GPUTexture texture) {
    if (texture.descriptor.imageView == VK_NULL_HANDLE || texture.descriptor.sampler == VK_NULL_HANDLE) {
        aout << "Error: Attempted to register invalid texture in ScreenSpaceRenderer!" << std::endl;
        return 0;
    }

    const auto& device = RENDER_DEVICE->getDevice();
    uint32_t slot = allocateTextureSlot();

    VkDescriptorImageInfo imageInfo = texture.descriptor;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorSet dstSet = m_bindlessRenderer.frameData[i].bindlessSet;
        if (dstSet == VK_NULL_HANDLE) {
            aout << "Error: Bindless set not initialized for frame " << i << " in ScreenSpaceRenderer!" << std::endl;
            continue;
        }

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = dstSet;
        write.dstBinding = 3;
        write.dstArrayElement = slot;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    return slot;
}

uint32_t ScreenSpaceRenderer::registerElement(GPUElementHandle element) {
    uint32_t index = m_bindlessRenderer.elements.size();
    m_bindlessRenderer.elements.push_back(element);
    return index;
}

void ScreenSpaceRenderer::updateElement(uint32_t index, GPUElementHandle element) {
    if (index < m_bindlessRenderer.elements.size()) {
        m_bindlessRenderer.elements[index] = element;
    } else {
        aout << "Error: Element index out of bounds!" << std::endl;
    }
}

bool ScreenSpaceRenderer::createBindlessDescriptors() {
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

    // binding 0 ObjectBuffer
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 1 PerFrame
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 3 textures[]
    bindings[2].binding = 3;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = MAX_SCREEN_TEXTURES;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorBindingFlags, 3> bindingFlags = {
            0, 0,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = 3;
    flagsInfo.pBindingFlags = bindingFlags.data();

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings.data();

    layoutInfo.pNext = &flagsInfo;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    /* Create Descriptor Set Layout */
    if (vkCreateDescriptorSetLayout(RENDER_DEVICE->getDevice(), &layoutInfo, nullptr,
                                    &m_bindlessRenderer.bindlessSetLayout) != VK_SUCCESS) {
        aout << "Failed to create descriptor set layout!" << std::endl;
        return false;
    }

    /* Create Descriptor Pool*/
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = MAX_SCREEN_TEXTURES * MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 1 * MAX_FRAMES_IN_FLIGHT;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = 1 * MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    if (vkCreateDescriptorPool(RENDER_DEVICE->getDevice(), &poolInfo, nullptr,
                               &m_bindlessRenderer.bindlessPool) != VK_SUCCESS) {
        aout << "Failed to create descriptor pool!" << std::endl;
        return false;
    }

    m_bindlessRenderer.textureSlotUsed.resize(MAX_SCREEN_TEXTURES, false);

    return true;
}

bool ScreenSpaceRenderer::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PCScreenElements);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_bindlessRenderer.bindlessSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(RENDER_DEVICE->getDevice(), &pipelineLayoutInfo, nullptr,
                               &m_bindlessRenderer.pipelineLayout) != VK_SUCCESS) {
        aout << "Failed to create pipeline layout!" << std::endl;
        return false;
    }

    return true;
}

bool ScreenSpaceRenderer::initializeFrames() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (!initializeFrame(m_bindlessRenderer.frameData[i])) {
            aout << "Failed to initialize frame!" << std::endl;
            return false;
        }
    }
    return true;
}

bool ScreenSpaceRenderer::initializeFrame(ScreenElements &frame) {
    const auto &device = RENDER_DEVICE->getDevice();

    /* Allocate command buffer*/
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = RENDER_DEVICE->getPrimaryCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &frame.cmd) != VK_SUCCESS) {
        return false;
    }

    /* Fence */
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(device, &fenceInfo, nullptr, &frame.fence) != VK_SUCCESS) {
        return false;
    }

    /* Create Buffers */
    if (!RenderFramework::createBuffer(sizeof(GPUElementHandle) * MAX_OBJECTS,
                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  &frame.meshBuffer.size, &frame.meshBuffer)) {
        return false;
    }

    if (!RenderFramework::createBuffer(sizeof(ScreenSpaceUBO),
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  &frame.perFrameBuffer.size, &frame.perFrameBuffer)) {
        return false;
    }

    /* Allocate Descriptor Set for this frame */
    uint32_t variableCount = MAX_SCREEN_TEXTURES;

    VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
    countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    countInfo.descriptorSetCount = 1;
    countInfo.pDescriptorCounts = &variableCount;

    VkDescriptorSetAllocateInfo dsAllocInfo{};
    dsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsAllocInfo.descriptorPool = m_bindlessRenderer.bindlessPool;
    dsAllocInfo.descriptorSetCount = 1;
    dsAllocInfo.pSetLayouts = &m_bindlessRenderer.bindlessSetLayout;
    dsAllocInfo.pNext = &countInfo;

    auto res = vkAllocateDescriptorSets(device, &dsAllocInfo, &frame.bindlessSet);

    if (res != VK_SUCCESS) {
        return false;
    }

    /* Update the descriptor set with the frame buffers */
    std::array<VkDescriptorBufferInfo, 2> bufferInfos = {};
    bufferInfos[0].buffer = frame.meshBuffer.buffer;
    bufferInfos[0].offset = 0;
    bufferInfos[0].range = VK_WHOLE_SIZE;

    bufferInfos[1].buffer = frame.perFrameBuffer.buffer;
    bufferInfos[1].offset = 0;
    bufferInfos[1].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> writes = {};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = frame.bindlessSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &bufferInfos[0];

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = frame.bindlessSet;
    writes[1].dstBinding = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[1].descriptorCount = 1;
    writes[1].pBufferInfo = &bufferInfos[1];

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    return true;
}

uint32_t ScreenSpaceRenderer::allocateTextureSlot() {
    for (uint32_t i = 0; i < MAX_SCREEN_TEXTURES; ++i) {
        if (!m_bindlessRenderer.textureSlotUsed[i]) {
            m_bindlessRenderer.textureSlotUsed[i] = true;
            return i;
        }
    }

    throw std::runtime_error("No texture slots available!");
}

VkDescriptorSet &ScreenSpaceRenderer::getBindLessSet(uint32_t frame) {
    return m_bindlessRenderer.frameData[frame].bindlessSet;
}

uint32_t ScreenSpaceRenderer::registerObject(GPUElementHandle object) {
    uint32_t index = m_bindlessRenderer.elements.size();
    m_bindlessRenderer.elements.push_back(object);
    return index;
}

GPUBuffer ScreenSpaceRenderer::createQuadBuffer(const glm::vec2 &origin, const glm::vec2 &size) {
    GPUBuffer vertexBuffer{};
    OGVertex2D vertices[4] = {};

    vertices[0].position = glm::vec2(origin.x, origin.y);
    vertices[0].uv = glm::vec2(1.0f, 1.0f);

    vertices[1].position = glm::vec2(origin.x + size.x, origin.y);
    vertices[1].uv = glm::vec2(1.0f, 0.0f);

    vertices[2].position = glm::vec2(origin.x + size.x, origin.y + size.y);
    vertices[2].uv = glm::vec2(0.0f,0.0f);

    vertices[3].position = glm::vec2(origin.x, origin.y + size.y);
    vertices[3].uv = glm::vec2(0.0f, 1.0f);

    if (!RenderFramework::createStagingBuffer(vertices, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer)) {
        aout << "Failed to create vertex buffer!" << std::endl;
    }

    return vertexBuffer;
}
