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
