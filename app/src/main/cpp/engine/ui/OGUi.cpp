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
uint32_t OGUi::addSprite(const std::string& spriteTextureName, glm::vec2 position, glm::vec2 scale) {
    uint32_t instanceId = -1;
    auto* spriteData = m_atlas.getSpriteData(spriteTextureName);
    if(spriteData) {
        SpriteInstance instance;
        instance.position = position;
        instance.size = scale;
        instance.uvOffset = {spriteData->x, spriteData->y};
        instance.uvScale = {spriteData->width, spriteData->height};
        instanceId = static_cast<uint32_t>(m_instances.size());
        m_instances.push_back(instance);
    }

    return instanceId;
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


std::vector<std::unique_ptr<OGElement>> &OGUi::getElements() {
    return m_elements;
}

const std::vector<std::unique_ptr<OGElement>> &OGUi::getElements() const {
    return m_elements;
}

const GPUTexture* OGUi::getAtlasTexture() {
    return m_atlas.getAtlasTexture();
}

const std::vector<SpriteInstance>& OGUi::getInstances() const {
    return m_instances;
}

void OGUi::clearInstances() {
    m_instances.clear();
}

void OGUi::clearElements() {
    m_elements.clear();
}

void OGUi::drawString(VkCommandBuffer cmd, const std::string &text, float x, float y, float scale) {
    m_fontEngine.renderString(cmd, text, x, y, scale);
}

void OGUi::addButton(OGButton *button) {
    m_buttons.push_back(button);
}

bool OGUi::handleInput(const glm::vec2 &touchPosition, bool pressed) {
    for (const auto& btn : m_buttons) {
        if (btn->handleInput(touchPosition, pressed)) {
            return true;
        }
    }
    return false;
}
