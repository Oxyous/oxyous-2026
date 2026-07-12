//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "GPUStaticMesh.hpp"
#include "../vulkan/RenderFramework.hpp"
#include "engine/collision/Collision.hpp"

/* Upload Data to GPU using buffers*/
bool
GPUStaticMesh::upload(std::vector<StaticMeshVertex> &vertices, std::vector<uint32_t> &indices) {
    if (!RenderFramework::createStagingBuffer(vertices.data(),
                                              sizeof(StaticMeshVertex) * vertices.size(),
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer)) {
        aout << "Failed to create vertex buffer" << std::endl;
        throw std::runtime_error("Failed to create vertex buffer");
    }

    if (!RenderFramework::createStagingBuffer(indices.data(), sizeof(uint32_t) * indices.size(),
                                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indexBuffer)) {
        aout << "Failed to create index buffer" << std::endl;
        throw std::runtime_error("Failed to create index buffer");
    }

    indexCount = indices.size();
    return true;
}

/* Render Mesh */
void GPUStaticMesh::render(VkCommandBuffer &commandBuffer) {
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &vertexBuffer.offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}

/* Load Vertices/Indexes from raw data */
bool GPUStaticMeshResource::load(AAssetManager *assetManager, const std::vector<uint8_t> &data) {

    auto dataPtr = data.data();

    /* Load Vertex Count */
    int vertexCount = *(int *) dataPtr;
    dataPtr += sizeof(int);

    /* Load Face Count */
    int indexCount = *(int *) dataPtr;
    dataPtr += sizeof(int);

    /** Load AABB min,max */
    float minx = *(float*) dataPtr;
    dataPtr += sizeof(float);

    float miny = *(float*) dataPtr;
    dataPtr += sizeof(float);

    float minz = *(float*)dataPtr;
    dataPtr += sizeof(float);

    float maxx = *(float*)dataPtr;
    dataPtr += sizeof(float);

    float maxy = *(float*)dataPtr;
    dataPtr += sizeof(float);

    float maxz = *(float*)dataPtr;
    dataPtr += sizeof(float);

    /** Allocate Bounds */
    AABBVolume volume = { {minx, miny, minz}, {maxx, maxy, maxz} };
    m_bounds = std::make_unique<AABBVolume>(volume);

    /* Load Vertices */
    std::vector<StaticMeshVertex> vertices(vertexCount);
    std::memcpy(vertices.data(), dataPtr, sizeof(StaticMeshVertex) * vertexCount);
    dataPtr += sizeof(StaticMeshVertex) * vertexCount;

    /* Load Face indexes */
    std::vector<uint32_t> indices(indexCount);
    std::memcpy(indices.data(), dataPtr, sizeof(uint32_t) * indexCount);
    dataPtr += sizeof(uint32_t) * indexCount;

    /* Upload Mesh Data to GPU */
    m_mesh = std::make_unique<GPUStaticMesh>();
    if (!m_mesh->upload(vertices, indices)) {
        aout << "Failed to upload mesh data to GPU" << std::endl;
        return false;
    }

    return true;
}

GPUStaticMesh *GPUStaticMeshResource::get() {
    return m_mesh.get();
}

void GPUStaticMeshResource::destroy() {
    if (m_mesh) {
        m_mesh.reset();
    }
}
