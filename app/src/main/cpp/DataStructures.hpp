//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#ifndef OXYOUS_2026_DATASTRUCTURES_HPP
#define OXYOUS_2026_DATASTRUCTURES_HPP

#include "includes.hpp"

class Renderer;

struct saveState {

};

/* App Engine Struct */
struct appEngine {
    int32_t width;
    int32_t height;
    struct android_app *app;
    struct saveState *state;
    Renderer *renderer;
};

/* GPUBuffer*/
typedef struct GPUBuffer {
    uint32_t binding = 0;
    uint32_t arrayElement = 0;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkDescriptorBufferInfo descriptor;
    VkDeviceSize offset = 0;
    VkDeviceSize range = 0;
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

    bool operator==(const GPUBuffer &other) const {
        return buffer == other.buffer &&
               offset == other.offset &&
               range == other.range &&
               memory == other.memory &&
               size == other.size &&
               descriptor.buffer == other.descriptor.buffer &&
               descriptor.offset == other.descriptor.offset &&
               descriptor.range == other.descriptor.range;
    }
} GPUBuffer;

/* GPU Image Struct */
typedef struct GPUImage {
    VkFormat format;
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory memory;

} GPUImage;

/* GPU Texture - GPU image with sampler */
typedef struct GPUTexture {
    uint32_t binding = 0;
    uint32_t arrayElement = 0;
    GPUImage image;
    VkSampler sampler;
    uint32_t width;
    uint32_t height;
    VkDescriptorImageInfo descriptor;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

    bool operator==(const GPUTexture &other) const {
        return image.image == other.image.image &&
               image.imageView == other.image.imageView &&
               image.memory == other.image.memory &&
               sampler == other.sampler &&
               width == other.width &&
               height == other.height &&
               descriptor.imageView == other.descriptor.imageView &&
               descriptor.sampler == other.descriptor.sampler &&
               descriptor.imageLayout == other.descriptor.imageLayout;
    }
} GPUTexture;

/* Static Mesh Vertex */
typedef struct StaticMeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec2 uv;
} StaticMeshVertex;

/* GPU Uniform Buffers Types */

/* Per Frame UBO */
typedef struct PerFrameUBO {
    glm::mat4 view;
    glm::mat4 projection;
} PerFrameUBO;

/* Per Object UBO */
typedef struct PerObjectUBO {
    glm::mat4 model;
} PerObjectUBO;

/* Post Process UBO*/
typedef struct PostProcessUBO {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 invView;
    glm::vec4 cameraPosition;
} PostProcessUBO;

#endif //OXYOUS_2026_DATASTRUCTURES_HPP
