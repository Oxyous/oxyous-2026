//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_UNIFORMBUFFER_HPP
#define OXYOUS_2026_UNIFORMBUFFER_HPP

#include "../RenderDevice.hpp"
#include "../RenderFramework.hpp"

class UniformBuffer {
public:
    /* Initialize Uniform Buffer */
    template<typename T>
    bool initialize(const T &data) {
        const auto &device = RENDER_DEVICE->getDevice();

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(T);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.flags = 0;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 0;
        bufferInfo.pQueueFamilyIndices = nullptr;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
            return false;
        }

        vkGetBufferMemoryRequirements(device, m_buffer, &m_memRequirements);

        /* Allocate Buffer memory*/
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = m_memRequirements.size;
        allocInfo.memoryTypeIndex = 0;

        if (!RenderFramework::findMemoryType(RENDER_DEVICE->getPhysicalDevice(),
                                             m_memRequirements.memoryTypeBits,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             allocInfo.memoryTypeIndex)) {
            return false;
        }

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
            aout << "Failed to allocate memory!" << std::endl;
            return false;
        }

        if (vkMapMemory(device, m_memory, 0, m_memRequirements.size, 0,
                         (void **) &m_mappedMemory) != VK_SUCCESS) {
            aout << "Failed to map memory!" << std::endl;
            return false;
        }

        memcpy(m_mappedMemory, &data, sizeof(T));

        vkUnmapMemory(device, m_memory);

        if (vkBindBufferMemory(device, m_buffer, m_memory, 0) != VK_SUCCESS) {
            aout << "Failed to bind buffer memory!" << std::endl;
            return false;
        }

        m_descriptorInfo.buffer = m_buffer;
        m_descriptorInfo.offset = 0;
        m_descriptorInfo.range = sizeof(T);

        return true;
    }

    /* Update Uniform Buffer */
    virtual void update(const void *data) {
        const auto &device = RENDER_DEVICE->getDevice();

        vkMapMemory(device, m_memory, 0, m_memRequirements.size, 0, (void **) &m_mappedMemory);
        memcpy(m_mappedMemory, data, m_memRequirements.size);
        vkUnmapMemory(device, m_memory);
    }

    /* Free GPU resources */
    virtual void destroy() {
        const auto &device = RENDER_DEVICE->getDevice();

        vkDestroyBuffer(device, m_buffer, nullptr);
        vkFreeMemory(device, m_memory, nullptr);
    }

    /* Get Buffer Descriptor info */
    virtual VkDescriptorBufferInfo *getDescriptorInfo() {
        return &m_descriptorInfo;
    }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
    VkDeviceSize m_size;
    VkBufferUsageFlags m_usage;
    VkMemoryRequirements m_memRequirements;
    VkDescriptorBufferInfo m_descriptorInfo;
    uint8_t *m_mappedMemory;
};

#endif //OXYOUS_2026_UNIFORMBUFFER_HPP
