//
// Created by Mr Steven J Baldwin on 20/07/2026.
//

#include "GPUSkeletalMesh.hpp"

void OGSkeletalMesh::buildBindPose(std::vector<OGJoint> &jointList) {

}

bool GPUSkeletalMeshResource::load(AAssetManager *assetManager, const std::vector<uint8_t> &data) {
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
        OGAnimMeshVertex vertex;
        uint32_t index;

        index = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        vertex.uv = *reinterpret_cast<const glm::vec2 *>(dataPtr);
        dataPtr += sizeof(glm::vec2);

        vertex.startWeight = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        vertex.weightCount = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        skeletonMesh.vertices.push_back(vertex);
    }

    /** Move to Faces offset*/
    dataPtr = static_cast<const uint8_t *>(meshData);
    dataPtr += header->faceOffset;

    for (int i=0;i<header->faceCount;i++) {
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

    for (int i=0;i<header->weightCount;i++) {
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

    for (int i=0;i<header->jointCount;i++) {
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

    return true;
}
