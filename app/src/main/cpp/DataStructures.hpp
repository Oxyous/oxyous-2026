//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#ifndef OXYOUS_2026_DATASTRUCTURES_HPP
#define OXYOUS_2026_DATASTRUCTURES_HPP

#include "includes.hpp"

class Renderer;

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint32_t MAX_OBJECTS = 65536;
constexpr uint32_t MAX_MATERIALS = 4096;
constexpr uint32_t MAX_LIGHTS = 256;
constexpr uint32_t MAX_TEXTURES = 4096;
constexpr uint32_t MAX_SCREEN_TEXTURES = 256;
constexpr float DESIGN_HEIGHT = 1080.0f;

struct saveState {

};

/* Polygon */
typedef struct OGPolygon {
    glm::vec3 vertices[3];
    glm::vec3 normal;
} OGPolygon;

/* Collision Contact Hit Result*/
typedef struct OGContact {
    glm::vec3 hitPoint;
    glm::vec3 normal;

    float depth;
} OGContact;

/* App Engine Struct */
struct appEngine {
    int32_t width;
    int32_t height;
    struct android_app *app;
    struct saveState *state;
    Renderer *renderer;
};

/** Sprite Data */
typedef struct OGSpriteData {
    std::string name;
    float x, y, width, height;
} OGSpriteData;

/** Sprite Mesh */
typedef struct OGSpriteMesh {
    uint32_t indexCount;
    uint32_t firstIndex;
    uint32_t vertexCount;
} OGSpriteMesh;

/** Single Quad Instanced */
typedef struct OGSpriteVertex {
    float x, y, u, v;
} OGSpriteVertex;

/** Single Quad Vertices */
const static OGSpriteVertex spriteVertices[] = {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f}
};

/** Single Quad Indices */
const static uint16_t spriteIndices[] =
        {
                0, 1, 2,
                2, 1, 3
        };

/** SpriteInstance */
typedef struct SpriteInstance {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 uvOffset;
    glm::vec2 uvScale;
}SpriteInstance;

/* */
typedef struct OGChar {
    uint32_t textureId;
    uint32_t objectId;
    glm::ivec2 size;
    glm::ivec2 bearing;
    uint32_t advance;
} OGChar;

/* */
typedef struct CSMData {
    glm::mat4 lightProjection[4];
    glm::vec4 cascadeSplits;
} CSMData;

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

    void *mapped;

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
    VkDescriptorImageInfo descriptor = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
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

/* 2d Vertex */
typedef struct OGVertex2D {
    glm::vec2 position;
    glm::vec2 uv;
} OGVertex2D;

/* GPU Uniform Buffers Types */

/* Per Frame UBO */
typedef struct PerFrameUBO {
    glm::mat4 view;
    glm::mat4 projection;
} PerFrameUBO;

/* Screen Space UBO*/
typedef struct ScreenSpaceUBO {
    glm::mat4 projection;
    glm::vec2 screenSize;
} ScreenSpaceUBO;

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

/* Post process CSM UBO */
typedef struct CSMUBO {
    glm::mat4 lightProjection[4];
    glm::vec4 cascadeSplits;
} CSMUBO;

/* Texture bindless handle */
typedef struct GPUTextureHandle {
    uint32_t index;         /* Index of the texture in the texture array */
    bool valid = false;     /* Whether the texture is valid or not */
} GPUTextureHandle;

/* Material bindless handle */
typedef struct GPUMaterialHandle {
    uint32_t albedoIndex;
    uint32_t normalIndex;
    uint32_t ormIndex;
    uint32_t emissiveIndex;

    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;

} GPUMaterialHandle;

/* Object GPU handle */
typedef struct GPUMeshHandle {
    glm::mat4 model;
    uint32_t materialIndex;
    uint32_t pad0;
    uint32_t pad1;
    uint32_t pad2;
} GPUMeshHandle;

/* Screen space handle */
typedef struct GPUElementHandle {
    glm::mat4 transform;
    uint32_t textureId;
    uint32_t pad0;
    uint32_t pad1;
    uint32_t pad2;
} GPUElementHandle;

/* Bindless Frame Data */
typedef struct FrameData {
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderComplete = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    VkDescriptorSet bindlessSet = VK_NULL_HANDLE;

    GPUBuffer meshBuffer;
    GPUBuffer materialBuffer;
    GPUBuffer perFrameBuffer;

    PerFrameUBO perFrame;

} FrameData;

/* Bindless Renderer */
typedef struct BindlessRenderer {
    VkDescriptorPool bindlessPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout bindlessSetLayout = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frameData{};
    uint32_t currentFrame = 0;

    std::vector<GPUMaterialHandle> materials;
    std::vector<GPUTextureHandle> textures;
    std::vector<GPUMeshHandle> meshes;
    std::vector<bool> textureSlotUsed;
} BindlessRenderer;

/* Bindless Push constants for mapping index to resource*/
typedef struct BindlessPushConstants {
    uint32_t materialIndex;
    uint32_t objectIndex;
} BindlessPushConstants;

/*  */
typedef struct ShadowMapPushConstants {
    uint32_t objectIndex;
    uint32_t cascadeIndex;
} ShadowMapPushConstants;

/* 2d element screen */
typedef struct ScreenElements {
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderComplete = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    VkDescriptorSet bindlessSet = VK_NULL_HANDLE;

    GPUBuffer meshBuffer;
    GPUBuffer perFrameBuffer;

    ScreenSpaceUBO perFrame;
} ScreenElements;

/* Bindless Renderer 2D Elements*/
typedef struct BindlessRenderer2D {
    VkDescriptorPool bindlessPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout bindlessSetLayout = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    std::array<ScreenElements, MAX_FRAMES_IN_FLIGHT> frameData{};
    uint32_t currentFrame = 0;

    std::vector<GPUElementHandle> elements;
    std::vector<GPUTextureHandle> textures;
    std::vector<bool> textureSlotUsed;
} BindlessRenderer2D;

typedef struct PCScreenElements {
    glm::mat4 transform;
    uint32_t textureIndex;
    uint32_t objectIndex;
} PCScreenElements;

#endif //OXYOUS_2026_DATASTRUCTURES_HPP
