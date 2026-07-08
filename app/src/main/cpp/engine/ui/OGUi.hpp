//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#ifndef OXYOUS_2026_OGUI_HPP
#define OXYOUS_2026_OGUI_HPP

#include "../../includes.hpp"
#include "../elements/OGElement.hpp"
#include "../../system/OGSingleton.hpp"
#include "render/text/FreeTypeFont.hpp"
#include "OGSpriteAtlas.hpp"

class OGUi {
public:
    OGUi() = default;

public:
    /* Initialize Font etc */
    bool initializeUI();

    template<typename T>
    T *addElement(T *element) {
        m_elements.emplace_back(element);
        return element;
    }

    void update(float delta) {
        for (auto &element: m_elements) {
            element->update(delta);
        }
    }

    std::vector<std::unique_ptr<OGElement>> &getElements() {
        return m_elements;
    }

    void
    drawString(VkCommandBuffer cmd, const std::string &text, float x, float y, float scale = 1.0f) {
        m_fontEngine.renderString(cmd, text, x, y, scale);
    }

    void clearElements() {
        m_elements.clear();
    }

    void addSprite(const std::string& spriteTextureName, glm::vec2 position, glm::vec2 scale);

    bool loadSpriteAsset(const std::string &assetPath, const std::string &spriteSheetName);

    /** Create Quad indices */
    std::vector<uint16_t> CreateQuadIndices (uint32_t maxSprites);

    GPUBuffer &getVertexBuffer() { return m_vertexBuffer; }

    GPUBuffer &getIndexBuffer() { return m_indexBuffer; }

    /**  */
    std::vector<SpriteInstance>& getInstances(){
        return m_instances;
    }

    OGSpriteAtlas getAtlas()  { return m_atlas; }

protected:
    std::vector<std::unique_ptr<OGElement>> m_elements;
    std::vector<SpriteInstance> m_instances;
    OGSpriteAtlas m_atlas;

    GPUBuffer m_vertexBuffer;
    GPUBuffer m_indexBuffer;

    FreeTypeFont m_fontEngine;
};

#define UI OGSingleton<OGUi>::getInstance()

#endif //OXYOUS_2026_OGUI_HPP
