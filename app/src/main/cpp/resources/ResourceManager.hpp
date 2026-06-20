//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#ifndef OXYOUS_2026_RESOURCEMANAGER_HPP
#define OXYOUS_2026_RESOURCEMANAGER_HPP


#include <android/asset_manager.h>
#include "../includes.hpp"
#include "../system/OGSingleton.hpp"

class ResourceManager {
public:
    ResourceManager();

    ~ResourceManager();

    [[nodiscard]] virtual AAssetManager* getAssetManager() const;

    void setAssetManager(AAssetManager *assetManager);

    /* Load Shader Binary form Assets*/
    void loadShader(const std::string& fileName, std::vector<uint8_t>& data);

private:
    AAssetManager* m_assetManager;
};

#define RESOURCE_MANAGER OGSingleton<ResourceManager>::getInstance()

#endif //OXYOUS_2026_RESOURCEMANAGER_HPP
