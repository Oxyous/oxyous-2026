//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#ifndef OXYOUS_2026_DESCRIPTORCACHE_HPP
#define OXYOUS_2026_DESCRIPTORCACHE_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"

inline void hashCombine(std::size_t &seed, std::size_t hash) {
    seed ^= hash + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
}

template<typename T>
inline std::uint64_t handleToUint64(const T &handle) {
    if constexpr (std::is_pointer_v<T>) {
        return reinterpret_cast<std::uint64_t>(handle);
    } else {
        return static_cast<std::uint64_t>(handle);
    }
}

/* Resource descriptions used as keys in the descriptor cache. */


struct DescriptorKey {
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<GPUBuffer> uniformBuffers;
    std::vector<GPUTexture> imageBindings;

    bool operator==(const DescriptorKey &other) const noexcept {
        return descriptorSetLayout == other.descriptorSetLayout &&
               uniformBuffers == other.uniformBuffers &&
               imageBindings == other.imageBindings;
    }

    void canonicalize() {
        auto bufLess = [](const GPUBuffer &a, const GPUBuffer &b) {
            if (a.buffer != b.buffer) return a.buffer < b.buffer;
            if (a.offset != b.offset) return a.offset < b.offset;
            return a.range < b.range;
        };

        auto imgLess = [](const GPUTexture &a, const GPUTexture &b) {
            if (a.image.image != b.image.image) return a.image.image < b.image.image;
            if (a.image.imageView != b.image.imageView) return a.image.imageView < b.image.imageView;
            if (a.image.memory != b.image.memory) return a.image.memory < b.image.memory;
            if (a.sampler != b.sampler) return a.sampler < b.sampler;
            if (a.width != b.width) return a.width < b.width;
            return a.height < b.height;
        };

        std::sort(uniformBuffers.begin(), uniformBuffers.end(), bufLess);
        std::sort(imageBindings.begin(), imageBindings.end(), imgLess);

    };
};

/* Descriptor Key Hasher */
struct DescriptorKeyHasher {
    std::size_t operator()(const DescriptorKey &key) const noexcept {
        std::size_t seed = 0;
        hashCombine(seed, handleToUint64(key.descriptorSetLayout));

        for (const auto &buffer: key.uniformBuffers) {
            hashCombine(seed, std::hash<std::uint32_t>{}(buffer.binding));
            hashCombine(seed, std::hash<std::uint32_t>{}(buffer.arrayElement));
            hashCombine(seed, std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(buffer.type)));
            hashCombine(seed, std::hash<std::uint64_t>{}(handleToUint64(buffer.buffer)));
            hashCombine(seed,
                        std::hash<std::uint64_t>{}(static_cast<std::uint64_t>(buffer.offset)));
            hashCombine(seed,
                        std::hash<std::uint64_t>{}(static_cast<std::uint64_t>(buffer.range)));
        }

        for (const auto &image: key.imageBindings) {
            hashCombine(seed, std::hash<std::uint32_t>{}(image.binding));
            hashCombine(seed, std::hash<std::uint32_t>{}(image.arrayElement));
            hashCombine(seed, std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(image.type)));
            hashCombine(seed, std::hash<std::uint64_t>{}(handleToUint64(image.sampler)));
            hashCombine(seed, std::hash<std::uint64_t>{}(handleToUint64(image.image.imageView)));
            hashCombine(seed,
                        std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(image.imageLayout)));
        }

        return seed;
    }
};

/* Descriptor Allocator  */
class DescriptorAllocator {
public:
    DescriptorAllocator() = default;

    bool initialize(VkDevice device, VkDescriptorPool descriptorPool) {
        if (descriptorPool == VK_NULL_HANDLE) {
            return false;
        }
        m_device = device;
        m_descriptorPool = descriptorPool;
    }

    [[nodiscard]] VkDescriptorSet allocate(VkDescriptorSetLayout descriptorSetLayout) const {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;

        VkDescriptorSet descriptorSet;
        if (vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set");
        }
        return descriptorSet;
    }

    void free(VkDescriptorSet descriptorSet) const {
        vkFreeDescriptorSets(m_device, m_descriptorPool, 1, &descriptorSet);
    }

    [[nodiscard]] VkDescriptorPool getDescriptorPool() const {
        return m_descriptorPool;
    }

private:
    VkDevice m_device;
    VkDescriptorPool m_descriptorPool;
};

/* Cached Descriptor Set */
struct CachedDescriptorSet {
    VkDescriptorSet descriptorSet;
    uint64_t lastUsedFrame;
};

/* Descriptor Cache */
class DescriptorCache {
public:
    DescriptorCache() = default;

    bool initialize(VkDevice device, VkDescriptorPool descriptorPool) {
        m_device = device;
        return m_descriptorAllocator.initialize(device, descriptorPool);
    }

    VkDescriptorSet fetch(DescriptorKey key, uint64_t currentFrame) {
        key.canonicalize();

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            it->second.lastUsedFrame = currentFrame;
            return it->second.descriptorSet;
        }

        auto descriptorSet = m_descriptorAllocator.allocate(key.descriptorSetLayout);

        writeDescriptorSet(descriptorSet, key);

        m_cache.emplace(std::move(key), CachedDescriptorSet{descriptorSet, currentFrame});
        return descriptorSet;
    }

    /**/
    void freeUnused(uint64_t currentFrame, uint32_t maxFrames) {
        for (auto it = m_cache.begin(); it != m_cache.end(); ) {
            if (currentFrame - it->second.lastUsedFrame > maxFrames) {
                m_descriptorAllocator.free(it->second.descriptorSet);
                it = m_cache.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    void writeDescriptorSet(VkDescriptorSet descriptorSet, const DescriptorKey &key) {
        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(key.uniformBuffers.size() + key.imageBindings.size());

        std::vector<VkDescriptorBufferInfo> bufferInfos;
        bufferInfos.reserve(key.uniformBuffers.size());

        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(key.imageBindings.size());

        for (const auto &buffer: key.uniformBuffers) {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = buffer.buffer;
            bufferInfo.offset = buffer.offset;
            bufferInfo.range = buffer.range;
            bufferInfos.push_back(bufferInfo);
        }

        for (size_t i = 0; i < bufferInfos.size(); ++i) {
            const auto &buffer = key.uniformBuffers[i];
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = buffer.binding;
            write.dstArrayElement = buffer.arrayElement;
            write.descriptorType = buffer.type;
            write.descriptorCount = 1;
            write.pBufferInfo = &bufferInfos[i];
            writes.push_back(write);
        }

        for (const auto &image: key.imageBindings) {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.sampler = image.sampler;
            imageInfo.imageView = image.image.imageView;
            imageInfo.imageLayout = image.imageLayout;
            imageInfos.push_back(imageInfo);
        }

        for (size_t i = 0; i < imageInfos.size(); ++i) {
            const auto &image = key.imageBindings[i];
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = image.binding;
            write.dstArrayElement = image.arrayElement;
            write.descriptorType = image.type;
            write.descriptorCount = 1;
            write.pImageInfo = &imageInfos[i];
            writes.push_back(write);
        }

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

private:
    VkDevice m_device{VK_NULL_HANDLE};
    DescriptorAllocator m_descriptorAllocator {};
    std::unordered_map<DescriptorKey, CachedDescriptorSet, DescriptorKeyHasher> m_cache;
};

#define DESCRIPTORS OGSingleton<DescriptorCache>::getInstance()

#endif //OXYOUS_2026_DESCRIPTORCACHE_HPP
