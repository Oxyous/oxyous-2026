//
// Created by Mr Steven J Baldwin on 06/07/2026.
//

#include "OGUi.hpp"
#include "render/vulkan/RenderFramework.hpp"

bool OGUi::initializeUI() {
    if (!m_fontEngine.initializeFont("Roboto_Condensed-Regular.ttf")) {
        throw std::runtime_error("Error: loading font");
        return false;
    }
    return true;
}

/** Create Sprite Vertex */
void OGUi::addSprite(const std::string& spriteTextureName, glm::vec2 position, glm::vec2 scale) {
    auto* spriteData = m_atlas.getSpriteData(spriteTextureName);
    if(spriteData) {
        SpriteInstance instance;
        instance.position = position;
        instance.size = scale;
        instance.uvOffset = {spriteData->x, spriteData->y};
        instance.uvScale = {spriteData->width, spriteData->height};
        m_instances.push_back(instance);
    }
}

bool OGUi::loadSpriteAsset(const std::string &assetPath, const std::string &spriteSheetName) {

    std::shared_ptr<OGSpriteAtlas> newAtlas = std::make_shared<OGSpriteAtlas>();

    if (!newAtlas->loadSprite(assetPath, spriteSheetName)) {
        aout << "Error: Failed to load sprite sheet: " << assetPath << std::endl;
        return false;
    }

    m_atlas = *newAtlas;
    return true;
}

/** Create Quad Indices */
std::vector<uint16_t> OGUi::CreateQuadIndices(uint32_t maxSprites) {
    std::vector<uint16_t> indices(maxSprites * 6);

    for (uint32_t i = 0; i < maxSprites; i++) {
        uint16_t baseVertex = static_cast<uint16_t>(i * 4);
        uint32_t index = i * 6;

        indices[index + 0] = baseVertex + 0;
        indices[index + 1] = baseVertex + 1;
        indices[index + 2] = baseVertex + 2;
        indices[index + 3] = baseVertex + 2;
        indices[index + 4] = baseVertex + 3;
        indices[index + 5] = baseVertex + 0;
    }

    return indices;
}
