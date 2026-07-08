//
// Created by Mr Steven J Baldwin on 08/07/2026.
//

#include "OGSpriteAtlas.hpp"
#include "resources/ResourceManager.hpp"
#include "engine/elements/ScreenSpaceRenderer.hpp"
#include "resources/GPUTextureResource.hpp"
#include "system/OGXml.hpp"

uint32_t OGSpriteAtlas::allocateSpriteTexture(GPUTexture spriteSheet) {
    return SCREEN_RENDER->registerTexture(spriteSheet);
}

bool OGSpriteAtlas::loadSprite(const std::string &assetPath, const std::string &spriteName) {
    const auto texture = RESOURCE_MANAGER->get<GPUTextureResource>(assetPath + spriteName + ".png")->get();
    if (texture) {
        m_atlasTexture = texture;
    }

    /* Load XML Sprite Sheet data */
    std::vector<std::unique_ptr<OGXmlNode>> nodes;

    if (!OGXml::loadGXml(spriteName + ".xml", nodes)) {
        aout << "Error: Failed to load sprite sheet XML: " << assetPath << std::endl;
        return false;
    }

    std::shared_ptr<OGXmlNode> root;

    for (const auto &node: nodes) {
        if (node->getName() == "Atlas") {
            root = std::shared_ptr<OGXmlNode>(node.get(), [](OGXmlNode *) {});
            break;
        }
    }

    if (!root) {
        return false;
    }

    for (const auto &childNode: root->getChildren()) {
        if (childNode->getName() == "Texture") {
            std::string textureName = "";
            float x = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float h = 0.0f;

            const auto &attrs = childNode->getAttributes();

            auto nameAttr = attrs.find("name");
            if (nameAttr != attrs.end()) {
                textureName = nameAttr->second;
            } else {
                throw std::runtime_error("Missing Attribute");
            }

            auto posX = attrs.find("x");
            if (posX != attrs.end()) {
                x = stof(posX->second);
            } else {
                throw std::runtime_error("Missing Attribute");
            }

            auto posY = attrs.find("y");
            if (posY != attrs.end()) {
                y = stof(posY->second);
            } else {
                throw std::runtime_error("Missing Attribute");
            }

            auto width = attrs.find("width");
            if (width != attrs.end()) {
                w = stof(width->second);
            } else {
                throw std::runtime_error("Missing Attribute");
            }

            auto height = attrs.find("height");
            if (height != attrs.end()) {
                h = stof(height->second);
            } else {
                throw std::runtime_error("Missing Attribute");
            }

            m_spritesData[textureName] = std::make_shared<OGSpriteData>();
            m_spritesData[textureName]->name = textureName;
            m_spritesData[textureName]->x = x;
            m_spritesData[textureName]->y = y;
            m_spritesData[textureName]->width = w;
            m_spritesData[textureName]->height = h;
        }
    }

    return true;
}

OGSpriteData* OGSpriteAtlas::getSpriteData(const std::string &spriteName) {
    auto it = m_spritesData.find(spriteName);
    if (it != m_spritesData.end()) {
        return it->second.get();
    }
    return {};
}

GPUTexture* OGSpriteAtlas::getAtlasTexture() {
    return m_atlasTexture;
}