//
// Created by Mr Steven J Baldwin on 25/07/2026.
//

#ifndef OXYOUS_2026_OGSKELETALMESHCOMPONENT_HPP
#define OXYOUS_2026_OGSKELETALMESHCOMPONENT_HPP


#include <stdint.h>
#include "engine/entity/OGObject.hpp"
#include "engine/entity/OGEntity.hpp"
#include "resources/GPUTextureResource.hpp"
#include "resources/GPUSkeletalMesh.hpp"

class OGSkeletalMeshComponent  : public OGComponent {
public:
    DEFINE_TYPE
    GET_UNIQUE_TYPE(OGSkeletalMeshComponent)
public:
    OGSkeletalMeshComponent();
    ~OGSkeletalMeshComponent();

    void initialize() override;

    void update(double deltaTime) override;

    void destroy() override;

    void render(VkCommandBuffer &commandBuffer, uint64_t currentFrame) override;

    void renderShadow(VkCommandBuffer &commandBuffer, uint64_t currentFrame, VkPipelineLayout layout, CSMData data, uint32_t cascade);

    OGEntity *getOwner() const override;

    std::shared_ptr<GPUSkeletalMeshResource> getMeshResource() const {
        return m_mesh;
    }

    uint32_t getBoneIndex() const {
        return m_boneIndex;
    }

    void setMeshResource(const std::shared_ptr<GPUSkeletalMeshResource> &mesh);

    void setMaterialIndex(uint32_t index);

    void setBoneIndex(uint32_t index) {
        m_boneIndex = index;
    }

private:
    std::shared_ptr<GPUSkeletalMeshResource> m_mesh;
    std::unordered_map<TEXTURE_SLOT, std::shared_ptr<GPUTextureResource>> m_textures;

    uint32_t m_objectIndex = 0xFFFFFFFF;
    uint32_t m_materialIndex = 0xFFFFFFFF;
    uint32_t m_boneIndex = 0;
};


#endif //OXYOUS_2026_OGSKELETALMESHCOMPONENT_HPP
