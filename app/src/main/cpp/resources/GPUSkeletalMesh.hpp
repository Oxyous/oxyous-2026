//
// Created by Mr Steven J Baldwin on 20/07/2026.
//

#ifndef OXYOUS_2026_GPUSKELETALMESH_HPP
#define OXYOUS_2026_GPUSKELETALMESH_HPP


#include "ResourceManager.hpp"
#include "engine/math/MathHelper.hpp"

class OGSkeletalMesh {
public:
    OGSkeletalMesh() = default;
    ~OGSkeletalMesh() = default;

    void setSkeletalMesh(std::shared_ptr<OGSkeletonMesh> skeletalMesh) {
        m_skeletalMesh = std::move(skeletalMesh);
    }

    void prepareMesh();

    std::shared_ptr<OGSkeletonMesh> getSkeletalMesh()  {
        return m_skeletalMesh;
    }

    const std::vector<glm::mat4>& getInverseBindPose() const {
        return m_inverseBindPose;
    }

    std::vector<glm::mat4> getJointMatrices()  {
        return m_jointMatrices;
    }

    GPUBuffer* getVertexBuffer() {
        return &vertexBuffer;
    }

    GPUBuffer* getIndexBuffer() {
        return &indexBuffer;
    }

    uint32_t* getIndexCount() {
        return &indexCount;
    }


private:
    std::shared_ptr<OGSkeletonMesh> m_skeletalMesh;
    std::vector<glm::mat4> m_bindPose;
    std::vector<glm::mat4> m_inverseBindPose;
    std::vector<glm::mat4> m_jointMatrices;

    GPUBuffer vertexBuffer;
    GPUBuffer indexBuffer;
    uint32_t indexCount;
};

/** GPU Skeletal Mesh Resource */
class GPUSkeletalMeshResource : public GPUResource<OGSkeletalMesh> {
public:
    GPUSkeletalMeshResource(const std::string &asset)
            : GPUResource<OGSkeletalMesh>(asset) {
        // Load the skeletal mesh from the provided data
    }

/** Get Skeletal Mesh Resource */
    OGSkeletalMesh *get() override {
        return m_skeletalMesh.get();
    }

    OGSkeletalMesh *getSkeletalMesh() {
        return m_skeletalMesh.get();
    }

    /** Load Skeletal Mesh Resource */
    bool load(AAssetManager *assetManager, const std::vector<uint8_t> &data) override;

    void destroy() override;

private:
    std::shared_ptr<OGSkeletalMesh> m_skeletalMesh;
};


#endif //OXYOUS_2026_GPUSKELETALMESH_HPP
