//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#include "GPUResources.hpp"
#include "../render/vulkan/RenderFramework.hpp"

bool GPUResources::initialize() {
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

void GPUResources::uploadFrameData(FrameData &frameData) {
    if (!m_bindlessRenderer.meshes.empty()) {
        std::memcpy(frameData.meshBuffer.mapped, m_bindlessRenderer.meshes.data(),
                    sizeof(GPUMeshHandle) * m_bindlessRenderer.meshes.size());
    }
    if (!m_bindlessRenderer.materials.empty()) {
        std::memcpy(frameData.materialBuffer.mapped, m_bindlessRenderer.materials.data(),
                    sizeof(GPUMaterialHandle) * m_bindlessRenderer.materials.size());
    }

    std::memcpy(frameData.perFrameBuffer.mapped, &frameData.perFrame, sizeof(PerFrameUBO));
}

uint32_t GPUResources::registerTexture(GPUTexture texture) {
    if (texture.descriptor.imageView == VK_NULL_HANDLE || texture.descriptor.sampler == VK_NULL_HANDLE) {
        aout << "Error: Attempted to register invalid texture in GPUResources!" << std::endl;
        return 0;
    }

    uint32_t slot = allocateTextureSlot();

    const auto& device = RENDER_DEVICE->getDevice();
    VkDescriptorImageInfo imageInfo = texture.descriptor;

    // Update ALL per-frame descriptor sets with this texture
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorSet dstSet = m_bindlessRenderer.frameData[i].bindlessSet;
        if (dstSet == VK_NULL_HANDLE) {
            aout << "Error: Bindless set not initialized for frame " << i << " in GPUResources!" << std::endl;
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

uint32_t GPUResources::registerMaterial(GPUMaterialHandle material) {
    uint32_t index = m_bindlessRenderer.materials.size();
    m_bindlessRenderer.materials.push_back(material);
    return index;
}

uint32_t GPUResources::registerObject(GPUMeshHandle object) {
    uint32_t index = m_bindlessRenderer.meshes.size();
    m_bindlessRenderer.meshes.push_back(object);
    return index;
}

void GPUResources::updateObject(uint32_t index, GPUMeshHandle object) {
    if (index < m_bindlessRenderer.meshes.size()) {
        m_bindlessRenderer.meshes[index] = object;
    }
}

uint32_t GPUResources::allocateTextureSlot() {
    for (uint32_t i = 0; i < MAX_TEXTURES; ++i) {
        if (!m_bindlessRenderer.textureSlotUsed[i]) {
            m_bindlessRenderer.textureSlotUsed[i] = true;
            return i;
        }
    }

    throw std::runtime_error("No texture slots available!");
}

bool GPUResources::createBindlessDescriptors() {
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

    // binding 0 ObjectBuffer
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 1 MaterialBuffer
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 2 PerFrame
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 3 textures[]
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = MAX_TEXTURES;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorBindingFlags, 4> bindingFlags = {
            0, 0, 0,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = 4;
    flagsInfo.pBindingFlags = bindingFlags.data();

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 4;
    layoutInfo.pBindings = bindings.data();
    layoutInfo.pNext = &flagsInfo;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    /* Create Descriptor Set Layout */
    if (vkCreateDescriptorSetLayout(RENDER_DEVICE->getDevice(), &layoutInfo, nullptr,
                                    &m_bindlessRenderer.bindlessSetLayout) != VK_SUCCESS) {
        aout << "Failed to create descriptor set layout!" << std::endl;
        return false;
    }

    /* Create Descriptor Pool */
    std::array<VkDescriptorPoolSize, 3> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = MAX_TEXTURES * MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 2 * MAX_FRAMES_IN_FLIGHT;
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

    m_bindlessRenderer.textureSlotUsed.resize(MAX_TEXTURES, false);

    return true;
}

bool GPUResources::createPipelineLayout() {
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BindlessPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
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

/* Initialize Frames */
bool GPUResources::initializeFrames() {
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (!initializeFrame(m_bindlessRenderer.frameData[i])) {
            aout << "Failed to initialize frame!" << std::endl;
            return false;
        }
    }
    return true;
}

/* Initialize Frame*/
bool GPUResources::initializeFrame(FrameData &frame) {
    const auto& device = RENDER_DEVICE->getDevice();

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

    RenderFramework::createBuffer(sizeof(GPUMeshHandle) * MAX_OBJECTS,
                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  &frame.meshBuffer.size, &frame.meshBuffer);

    RenderFramework::createBuffer(sizeof(GPUMaterialHandle) * MAX_MATERIALS,
                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  &frame.materialBuffer.size, &frame.materialBuffer);

    RenderFramework::createBuffer(sizeof(PerFrameUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &frame.perFrameBuffer.size,
                                  &frame.perFrameBuffer);

    /* Allocate Descriptor Set for this frame */
    uint32_t variableCount = MAX_TEXTURES;
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
    std::array<VkDescriptorBufferInfo, 3> bufferInfos = {};
    bufferInfos[0] = { .buffer = frame.meshBuffer.buffer, .offset = 0, .range = VK_WHOLE_SIZE };
    bufferInfos[1] = { .buffer = frame.materialBuffer.buffer, .offset = 0, .range = VK_WHOLE_SIZE };
    bufferInfos[2] = { .buffer = frame.perFrameBuffer.buffer, .offset = 0, .range = VK_WHOLE_SIZE };

    std::array<VkWriteDescriptorSet, 3> writes = {};
    for (int i = 0; i < 3; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = frame.bindlessSet;
        writes[i].dstBinding = i; // binding 0, 1, 2
        writes[i].dstArrayElement = 0;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = (i < 2) ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[i].pBufferInfo = &bufferInfos[i];
    }

    vkUpdateDescriptorSets(device, 3, writes.data(), 0, nullptr);

    return true;
}

/* Get Frame Data */
FrameData& GPUResources::getFrameData(uint32_t frame) {
    return m_bindlessRenderer.frameData[frame];
}

/* Bindless Pipeline Layout */
VkPipelineLayout &GPUResources::getPipelineLayout() {
    return m_bindlessRenderer.pipelineLayout;
}

/* Bindless Set Layout */
VkDescriptorSetLayout &GPUResources::getBindlessSetLayout() {
    return m_bindlessRenderer.bindlessSetLayout;
}

/* Bindless Descriptor Set */
VkDescriptorSet &GPUResources::getBindlessSet(uint32_t frame) {
    return m_bindlessRenderer.frameData[frame].bindlessSet;
}
