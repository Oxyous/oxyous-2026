//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#include "ResourceManager.hpp"

ResourceManager::ResourceManager() {
}

AAssetManager *ResourceManager::getAssetManager() const {
    return m_assetManager;
}

void ResourceManager::setAssetManager(AAssetManager *assetManager) {
    m_assetManager = assetManager;
}

ResourceManager::~ResourceManager() {

}
/**/
void ResourceManager::loadShader(const std::string& fileName, std::vector<uint8_t>& data) {
    AAsset* asset = AAssetManager_open(m_assetManager, fileName.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        aout << "Error: Failed to open asset" << std::endl;
        return;
    }

    off_t length = AAsset_getLength(asset);
    data.resize(length);
    int read = AAsset_read(asset, data.data(), length);

    if (read != length) {
        AAsset_close(asset);
        aout << "Error: Failed to read asset" << std::endl;
        return;
    }

    AAsset_close(asset);
}

/* Load Scene collision */
bool ResourceManager::loadSceneCollision(const std::string& assetPath, std::vector<OGPolygon>& polys)
{
    AAsset* asset = AAssetManager_open(m_assetManager, assetPath.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        aout << "Error: Failed to open collision asset: " << assetPath << std::endl;
        return false;
    }

    size_t size = AAsset_getLength(asset);
    std::vector<uint8_t> buffer(size);
    AAsset_read(asset, buffer.data(), size);

    auto pData = buffer.data();
    uint32_t vertexCount, normalCount, indexCount;
    uint32_t vertexOffset, normalOffset, indexOffset;

    vertexCount = *(uint32_t*)pData;
    pData += sizeof(uint32_t);
    normalCount = *(uint32_t*)pData;
    pData += sizeof(uint32_t);
    indexCount = *(uint32_t*)pData;
    pData += sizeof(uint32_t);

    vertexOffset = *(uint32_t*)pData;
    pData += sizeof(uint32_t);
    normalOffset = *(uint32_t*)pData;
    pData += sizeof(uint32_t);
    indexOffset = *(uint32_t*)pData;
    pData += sizeof(uint32_t);

    float* vertices = (float*)(buffer.data() + vertexOffset);
    float* normals = (float*)(buffer.data() + normalOffset);
    uint32_t* indices = (uint32_t*)(buffer.data() + indexOffset);

    for (uint32_t i = 0; i < indexCount; i += 3) {
        OGPolygon poly;
        for (int j = 0; j < 3; ++j) {
            uint32_t idx = indices[i + j];
            poly.vertices[j] = {
                vertices[idx * 3],
                vertices[idx * 3 + 1],
                vertices[idx * 3 + 2]
            };
        }
        glm::vec3 normal = glm::cross(poly.vertices[1] - poly.vertices[0], poly.vertices[2] - poly.vertices[0]);
        poly.normal = glm::normalize(normal);
        polys.push_back(poly);
    }

    AAsset_close(asset);

    return true;
}
