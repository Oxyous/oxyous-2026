//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_GPUSTATICMESH_HPP
#define OXYOUS_2026_GPUSTATICMESH_HPP


#include "GPUMesh.hpp"
#include "../../DataStructures.hpp"
#include "../../resources/ResourceManager.hpp"

class GPUStaticMesh: public GPUMesh {
public:
    GPUStaticMesh() = default;
    virtual ~GPUStaticMesh() = default;

    /* Upload Data to GPU */
    virtual bool upload(std::vector<StaticMeshVertex> &vertices, std::vector<uint32_t> &indices);

    /* */
    void render(VkCommandBuffer& commandBuffer) override;

private:
    GPUBuffer vertexBuffer;
    GPUBuffer indexBuffer;
    uint32_t indexCount;
};

/* Static Mesh Resource */
class GPUStaticMeshResource : public GPUResource<GPUStaticMesh>
{
public:
    GPUStaticMeshResource(const std::string& assetPath) : GPUResource<GPUStaticMesh>(assetPath) {}
    virtual ~GPUStaticMeshResource() = default;
public:

    /* */
    bool load(AAssetManager *assetManager, const std::vector<uint8_t> &data) override;

    /* */
    virtual GPUStaticMesh* get() override;

    /* */
    virtual void destroy() override;

private:
    std::unique_ptr<GPUStaticMesh> m_mesh;
};

#endif //OXYOUS_2026_GPUSTATICMESH_HPP
