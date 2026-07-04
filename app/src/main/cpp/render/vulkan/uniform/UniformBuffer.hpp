//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_UNIFORMBUFFER_HPP
#define OXYOUS_2026_UNIFORMBUFFER_HPP

#include "../RenderDevice.hpp"
#include "../RenderFramework.hpp"

class UniformBuffer {
public:
    /* Initialize Uniform Buffer - always creates 2 for double buffering */
    template<typename T>
    bool initialize(const T &data) {
        const auto &device = RENDER_DEVICE->getDevice();
        m_size = sizeof(T);

        for (int i = 0; i < 2; i++) {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = m_size;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.flags = 0;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffers[i]) != VK_SUCCESS) {
                return false;
            }

            vkGetBufferMemoryRequirements(device, m_buffers[i], &m_memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = m_memRequirements.size;

            if (!RenderFramework::findMemoryType(RENDER_DEVICE->getPhysicalDevice(),
                                                 m_memRequirements.memoryTypeBits,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                 allocInfo.memoryTypeIndex)) {
                return false;
            }

            if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memories[i]) != VK_SUCCESS) {
                return false;
            }

            if (vkMapMemory(device, m_memories[i], 0, m_memRequirements.size, 0,
                             (void **) &m_mappedMemories[i]) != VK_SUCCESS) {
                return false;
            }

            memcpy(m_mappedMemories[i], &data, m_size);

            if (vkBindBufferMemory(device, m_buffers[i], m_memories[i], 0) != VK_SUCCESS) {
                return false;
            }

            m_descriptorInfos[i].buffer = m_buffers[i];
            m_descriptorInfos[i].offset = 0;
            m_descriptorInfos[i].range = m_size;
        }

        return true;
    }

    /* Update Uniform Buffer for a specific frame index */
    virtual void update(const void *data, uint32_t frameIndex) {
        if (frameIndex >= 2) return;
        memcpy(m_mappedMemories[frameIndex], data, m_size);
    }

    /* Free GPU resources */
    virtual void destroy() {
        const auto &device = RENDER_DEVICE->getDevice();

        for (int i = 0; i < 2; i++) {
            if (m_buffers[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, m_buffers[i], nullptr);
                m_buffers[i] = VK_NULL_HANDLE;
            }
            if (m_memories[i] != VK_NULL_HANDLE) {
                vkUnmapMemory(device, m_memories[i]);
                vkFreeMemory(device, m_memories[i], nullptr);
                m_memories[i] = VK_NULL_HANDLE;
            }
        }
    }

    /* Get Buffer Descriptor info for a specific frame index */
    virtual VkDescriptorBufferInfo *getDescriptorInfo(uint32_t frameIndex) {
        return &m_descriptorInfos[frameIndex % 2];
    }

private:
    VkBuffer m_buffers[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_memories[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceSize m_size{0};
    VkMemoryRequirements m_memRequirements{};
    VkDescriptorBufferInfo m_descriptorInfos[2]{};
    uint8_t *m_mappedMemories[2]{nullptr, nullptr};
};

#endif //OXYOUS_2026_UNIFORMBUFFER_HPP
