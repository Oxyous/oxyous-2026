//
// Created by Mr Steven J Baldwin on 20/07/2026.
//

#ifndef OXYOUS_2026_GPUSKELETALMESH_HPP
#define OXYOUS_2026_GPUSKELETALMESH_HPP


#include "ResourceManager.hpp"
#include "engine/math/MathHelper.hpp"
#include "animation/OGSkeletalAnimation.hpp"

class OGSkeletalMesh {
public:
    OGSkeletalMesh() = default;
    ~OGSkeletalMesh() = default;

    void setSkeletalMesh(std::shared_ptr<OGSkeletonMesh> skeletalMesh) {
        m_skeletalMesh = std::move(skeletalMesh);
    }

    /** Build Bind Pose */
    void buildBindPose(std::vector<OGJoint>& jointList);

private:
    std::shared_ptr<OGSkeletonMesh> m_skeletalMesh;
    std::vector<glm::mat4> m_bindPose;
    std::vector<glm::mat4> m_inverseBindPose;
    std::vector<glm::mat4> m_jointMatrices;

    std::shared_ptr<OGSkeletalAnimation> m_animation;
};

/** GPU Skeletal Mesh Resource */
class GPUSkeletalMeshResource : public GPUResource<OGSkeletalMesh> {
public:
    GPUSkeletalMeshResource(const std::string &asset, const std::vector<uint8_t> &data)
            : GPUResource<OGSkeletalMesh>(asset) {
        // Load the skeletal mesh from the provided data
    }

    /** Get Skeletal Mesh Resource */
    OGSkeletalMesh *get() override {
        return m_skeletalMesh.get();
    }

    /** Load Skeletal Mesh Resource */
    bool load(AAssetManager *assetManager, const std::vector<uint8_t> &data) override;

private:
    std::shared_ptr<OGSkeletalMesh> m_skeletalMesh;
};


#endif //OXYOUS_2026_GPUSKELETALMESH_HPP
