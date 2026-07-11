//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#ifndef OXYOUS_2026_GPURESOURCES_HPP
#define OXYOUS_2026_GPURESOURCES_HPP

#include "../includes.hpp"
#include "../DataStructures.hpp"

class GPUResources {
public:
    GPUResources() = default;

    bool initialize();

    void clear() {
        m_bindlessRenderer.meshes.clear();
        m_bindlessRenderer.materials.clear();
        m_bindlessRenderer.textures.clear();
        m_bindlessRenderer.textureSlotUsed.assign(MAX_TEXTURES, false);
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

    /* Update object data */
    void updateObject(uint32_t index, GPUMeshHandle object);

    /* */
    FrameData &getFrameData(uint32_t frame);

    /* */
    VkPipelineLayout &getPipelineLayout();

    /* */
    VkDescriptorSetLayout &getBindlessSetLayout();

    /**/
    VkDescriptorSet &getBindlessSet(uint32_t frame);

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
};

#define GPU_RESOURCES OGSingleton<GPUResources>::getInstance()

#endif //OXYOUS_2026_GPURESOURCES_HPP
