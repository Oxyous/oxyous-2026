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
#include "engine/elements/OGButton.hpp"

class UIWidget;
class UILayer;

class OGUi {
public:
    OGUi() = default;

public:
    /** Initialize Font etc */
    bool initializeUI();

    /** Add New UI Element*/
    template<typename T>
    T *addElement(T *element) {
        m_elements.emplace_back(element);
        return element;
    }

    /** Update All UI Elements*/
    void update(float delta) {
        for (auto &element: m_elements) {
            element->update(delta);
        }
    }

    /** Get All UI Elements */
    std::vector<std::unique_ptr<OGElement>> &getElements();

    /** Get All UI Elements (Const) */
    const std::vector<std::unique_ptr<OGElement>> &getElements() const;

    /** Draw String */
    void drawString(VkCommandBuffer cmd, const std::string &text, float x, float y, float scale = 1.0f, glm::vec4 colorAlpha = glm::vec4(1.0f));

    /** Remove All Elements*/
    void clearElements();

    /** Clear All Sprite Instances */
    void clearInstances();

    /** Add Sprite Texture Atlas */
    uint32_t addSprite(const std::string &spriteTextureName, glm::vec2 position, glm::vec2 scale, glm::vec4 colorAlpha = glm::vec4(1.0f));

    /** Load Sprite Asset */
    bool loadSpriteAsset(const std::string &assetPath, const std::string &spriteSheetName);

    /** Get All Sprite Instances  */
    const std::vector<SpriteInstance> &getInstances() const;

    /** Fetch Texture Atlas */
    const GPUTexture* getAtlasTexture();

    /** Get Vertex Buffer */
    GPUBuffer &getVertexBuffer() { return m_vertexBuffer; }

    /** Get Index Buffer */
    GPUBuffer &getIndexBuffer() { return m_indexBuffer; }

    /** Add UI Button Element */
    void addButton(OGButton* button);

    /** Handle Input for UI Elements */
    bool handleInput(const glm::vec2& touchPosition, bool pressed);

    /** Create a new UI layer */
    UILayer & addLayer(const std::string& name);

    /** Get a UI layer by name */
    UILayer* getLayer(const std::string& name);

    /** Get all UI layers */
    std::vector<std::shared_ptr<UILayer>>& getLayers() { return m_layers; }

protected:
    std::vector<std::unique_ptr<OGElement>> m_elements;
    std::vector<OGButton*> m_buttons;
    std::vector<SpriteInstance> m_instances;
    std::vector<std::shared_ptr<UILayer>> m_layers;
    OGSpriteAtlas m_atlas;

    std::vector<std::unique_ptr<UIWidget>> m_widgets;

    GPUBuffer m_vertexBuffer;
    GPUBuffer m_indexBuffer;
private:
    FreeTypeFont m_fontEngine;
};

#define UI OGSingleton<OGUi>::getInstance()

#include "UILayer.hpp"

#endif //OXYOUS_2026_OGUI_HPP
