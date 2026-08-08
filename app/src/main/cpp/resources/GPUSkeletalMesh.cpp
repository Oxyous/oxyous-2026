//
// Created by Mr Steven J Baldwin on 20/07/2026.
//

#include "GPUSkeletalMesh.hpp"
#include "render/vulkan/RenderFramework.hpp"

bool GPUSkeletalMeshResource::load(AAssetManager *assetManager) {
    auto meshAsset = AAssetManager_open(assetManager, m_assetPath.c_str(), AASSET_MODE_BUFFER);

    if (!meshAsset) {
        aout << "Failed to open skeletal mesh asset: " << m_assetPath << std::endl;
        return false;
    }

    auto meshData = AAsset_getBuffer(meshAsset);
    auto meshSize = AAsset_getLength(meshAsset);
    auto dataPtr = static_cast<const uint8_t *>(meshData);

    OGSkeletonMesh skeletonMesh;

    OGAnimMeshHeader *header = reinterpret_cast<OGAnimMeshHeader *>(const_cast<uint8_t *>(dataPtr));
    dataPtr += sizeof(OGAnimMeshHeader);

    dataPtr = static_cast<const uint8_t *>(meshData);
    dataPtr += header->vertexOffset;

    for (int i = 0; i < header->vertexCount; i++) {
        OGSkeletalVertex vertex;
        uint32_t index;

        index = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        vertex.uv = *reinterpret_cast<const glm::vec2 *>(dataPtr);
        dataPtr += sizeof(glm::vec2);

        vertex.startWeight = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        vertex.weightCount = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        vertex.tangent.x = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        vertex.tangent.y = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        vertex.tangent.z = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        vertex.tangent.w = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        skeletonMesh.vertices.push_back(vertex);
    }

    /** Move to Faces offset*/
    dataPtr = static_cast<const uint8_t *>(meshData);
    dataPtr += header->faceOffset;

    for (int i = 0; i < header->faceCount; i++) {
        OGFace face;
        uint32_t faceIndex = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        face.indices[0] = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        face.indices[1] = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        face.indices[2] = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        skeletonMesh.faces.push_back(face);
        skeletonMesh.indices.push_back(face.indices[0]);
        skeletonMesh.indices.push_back(face.indices[1]);
        skeletonMesh.indices.push_back(face.indices[2]);
    }

    /** Move Pointer to Weight offset */
    dataPtr = static_cast<const uint8_t *>(meshData);
    dataPtr += header->weightOffset;

    for (int i = 0; i < header->weightCount; i++) {
        OGWeight weight;
        uint32_t index;

        index = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        weight.jointIndex = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        weight.bias = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        weight.position = *reinterpret_cast<const glm::vec3 *>(dataPtr);
        dataPtr += sizeof(glm::vec3);

        skeletonMesh.weights.push_back(weight);
    }

    /** Move Pointer to Joints offset */
    dataPtr = static_cast<const uint8_t *>(meshData);
    dataPtr += header->jointOffset;

    for (int i = 0; i < header->jointCount; i++) {
        OGJoint joint;
        uint32_t nameLength;
        float orientation[3] = {0.0f, 0.0f, 0.0f};

        nameLength = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        joint.name.resize(nameLength);
        std::memcpy(joint.name.data(), dataPtr, nameLength);
        dataPtr += nameLength;

        joint.parentIndex = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        joint.position = *reinterpret_cast<const glm::vec3 *>(dataPtr);
        dataPtr += sizeof(glm::vec3);

        orientation[0] = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        orientation[1] = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        orientation[2] = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        joint.orientation = glm::quat(0.0f, orientation[0], orientation[1], orientation[2]);

        Math::computeQuaternion(joint.orientation);

        skeletonMesh.joints.push_back(joint);
    }
    AAsset_close(meshAsset);

    m_skeletalMesh = std::make_shared<OGSkeletalMesh>();
    m_skeletalMesh->setSkeletalMesh(std::make_shared<OGSkeletonMesh>(skeletonMesh));
    m_skeletalMesh->prepareMesh();

    return true;
}

void GPUSkeletalMeshResource::destroy() {

}

void OGSkeletalMesh::prepareMesh() {
    size_t numJoints = m_skeletalMesh->joints.size();
    m_bindPose.resize(numJoints);
    m_inverseBindPose.resize(numJoints);
    std::vector<OGJoint> globalJoints(numJoints);

    // 1. Compute Global Bind Pose
    for (int i = 0; i < numJoints; i++) {
        auto &joint = m_skeletalMesh->joints[i];

        m_bindPose[i] = glm::translate(glm::mat4(1.0f), joint.position) *
                        glm::mat4_cast(joint.orientation);
        m_inverseBindPose[i] = glm::inverse(m_bindPose[i]);
    }

    m_skeletalMesh->skinnedVertices.clear();
    m_skeletalMesh->skinnedVertices.reserve(m_skeletalMesh->vertices.size());

    // 2. Compute Bind-Pose Mesh (Global space)
    for (int i = 0; i < m_skeletalMesh->vertices.size(); i++) {
        auto &vertex = m_skeletalMesh->vertices[i];

        OGSkeletalVertex bindVertex = vertex;
        bindVertex.position = glm::vec3(0.0f);
        bindVertex.normal = glm::vec3(0.0f);
        bindVertex.boneWeights = glm::vec4(0.0f);
        bindVertex.boneIndices = glm::ivec4(0);

        for (int j = 0; j < vertex.weightCount; j++) {
            if (vertex.startWeight + j >= (int)m_skeletalMesh->weights.size()) break;
            auto &weight = m_skeletalMesh->weights[vertex.startWeight + j];
            if (weight.jointIndex < 0 || weight.jointIndex >= (int)numJoints) continue;

            auto &joint = m_skeletalMesh->joints[weight.jointIndex];

            // Reconstruct position in Global Bind Pose
            glm::vec3 rotPos = joint.orientation * weight.position;
            bindVertex.position += (joint.position + rotPos) * weight.bias;

            if (j < 4) {
                bindVertex.boneIndices[j] = weight.jointIndex;
                bindVertex.boneWeights[j] = weight.bias;
            }
        }

        // Ensure weights sum to 1
        float totalWeight = bindVertex.boneWeights.x + bindVertex.boneWeights.y + bindVertex.boneWeights.z + bindVertex.boneWeights.w;
        if (totalWeight > 0.0f) {
            bindVertex.boneWeights /= totalWeight;
        }

        m_skeletalMesh->skinnedVertices.push_back(bindVertex);
    }

    // 3. Compute Normals for the Bind Pose Mesh
    for (int i = 0; i < m_skeletalMesh->faces.size(); i++) {
        auto &face = m_skeletalMesh->faces[i];
        if (face.indices[0] >= m_skeletalMesh->skinnedVertices.size() ||
            face.indices[1] >= m_skeletalMesh->skinnedVertices.size() ||
            face.indices[2] >= m_skeletalMesh->skinnedVertices.size()) continue;

        auto &v0 = m_skeletalMesh->skinnedVertices[face.indices[0]];
        auto &v1 = m_skeletalMesh->skinnedVertices[face.indices[1]];
        auto &v2 = m_skeletalMesh->skinnedVertices[face.indices[2]];

        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;
        glm::vec3 normal = glm::cross(edge1, edge2);

        v0.normal += normal;
        v1.normal += normal;
        v2.normal += normal;
    }

    for (auto &vertex : m_skeletalMesh->skinnedVertices) {
        if (glm::length(vertex.normal) > 0.0001f) {
            vertex.normal = glm::normalize(vertex.normal);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        vertex.tangent = glm::vec4(glm::normalize(glm::vec3(vertex.tangent)), vertex.tangent.w);
    }

    // 4. Upload Bind Pose Mesh to GPU
    if (!RenderFramework::createStagingBuffer(m_skeletalMesh->skinnedVertices.data(),
                                              sizeof(OGSkeletalVertex) * m_skeletalMesh->skinnedVertices.size(),
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer)) {
        aout << "Failed to create vertex buffer" << std::endl;
        throw std::runtime_error("Failed to create vertex buffer");
    }

    if (!RenderFramework::createStagingBuffer(m_skeletalMesh->indices.data(), sizeof(uint32_t) * m_skeletalMesh->indices.size(),
                                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indexBuffer)) {
        aout << "Failed to create index buffer" << std::endl;
        throw std::runtime_error("Failed to create index buffer");
    }

    indexCount = m_skeletalMesh->indices.size();
}
