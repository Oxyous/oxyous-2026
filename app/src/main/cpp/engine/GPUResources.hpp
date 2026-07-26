//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#ifndef OXYOUS_2026_GPURESOURCES_HPP
#define OXYOUS_2026_GPURESOURCES_HPP

#include "../includes.hpp"
#include "../DataStructures.hpp"
#include <mutex>

class GPUResources {
public:
    GPUResources() = default;

    bool initialize();

    void clear() {
        m_bindlessRenderer.meshes.clear();
        m_bindlessRenderer.materials.clear();
        m_bindlessRenderer.textures.clear();
        m_bindlessRenderer.textureSlotUsed.assign(MAX_TEXTURES, false);
        m_nextBoneSlot = 0;
        std::fill(m_dirtyBones.begin(), m_dirtyBones.end(), 0);
    }
public:

    /* */
    void uploadFrameData(FrameData& frameData);

    /* Register Texture for bindless*/
    uint32_t registerTexture(GPUTexture texture);

    /* Register material for bindless */
    uint32_t registerMaterial(GPUMaterialHandle material);

    /* register object for bindless */
    uint32_t registerObject(GPUMeshHandle object);

    /* Register bone block */
    uint32_t registerBoneBlock();

    /* Update object data */
    void updateObject(uint32_t index, GPUMeshHandle object);

    /* Update bone data */
    void updateBones(uint32_t boneIndex, const std::vector<glm::mat4>& matrices);

    /* */
    FrameData &getFrameData(uint32_t frame);

    /* */
    VkPipelineLayout &getPipelineLayout();

    /* */
    VkDescriptorSetLayout &getBindlessSetLayout();

    /* */
    VkDescriptorSetLayout &getBoneSetLayout();

    /**/
    VkDescriptorSet &getBindlessSet(uint32_t frame);

    /**/
    VkDescriptorSet &getBoneSet(uint32_t frame);

    std::mutex& getMutex() { return m_resourceMutex; }

protected:
    /* */
    uint32_t allocateTextureSlot();

    /* */
    bool createBindlessDescriptors();

    /* */
    bool createPipelineLayout();

    bool initializeFrames();

    bool initializeFrame(FrameData& frame);

protected:
    FrameData m_frameData;
    BindlessRenderer m_bindlessRenderer;
    std::mutex m_resourceMutex;
    uint32_t m_nextBoneSlot = 0;
    std::vector<uint32_t> m_dirtyBones;
};

#define GPU_RESOURCES OGSingleton<GPUResources>::getInstance()

#endif //OXYOUS_2026_GPURESOURCES_HPP
