//
// Created by Mr Steven J Baldwin on 08/07/2026.
//

#ifndef OXYOUS_2026_OGSPRITEATLAS_HPP
#define OXYOUS_2026_OGSPRITEATLAS_HPP

#include <string>
#include <map>
#include "../../includes.hpp"
#include "DataStructures.hpp"

class OGSpriteAtlas {
public:
    /** */
    bool loadSprite(const std::string& assetPath, const std::string& spriteName);

    /**  */
    uint32_t allocateSpriteTexture(GPUTexture spriteSheet);

    /** */
    OGSpriteData* getSpriteData(const std::string& spriteName);

    /** */
    GPUTexture* getAtlasTexture() ;

private:
    GPUTexture* m_atlasTexture = nullptr;
    std::map<std::string, std::shared_ptr<OGSpriteData>> m_spritesData;
};


#endif //OXYOUS_2026_OGSPRITEATLAS_HPP
