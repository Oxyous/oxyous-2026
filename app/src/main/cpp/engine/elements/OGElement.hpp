//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#ifndef OXYOUS_2026_OGELEMENT_HPP
#define OXYOUS_2026_OGELEMENT_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"

class OGElement {
public:
    virtual void draw(VkCommandBuffer& commandBuffer, uint32_t frame) = 0;

    virtual void destroy() = 0;

    virtual void update(float delta) = 0;

    virtual bool handleInput(const glm::vec2& touchPosition, bool pressed) = 0;

    virtual glm::mat4 getTransform() {
        return m_translation * m_size;
    }

    virtual bool isVisible() {
        return m_visible;
    }

    virtual void setTranslation(const glm::vec3 &translation) {
        m_translation = glm::translate(glm::mat4(1.0f), translation);
    }

    virtual void setVisible(bool visible) {
        m_visible = visible;
    }

protected:
    GPUBuffer m_vertexBuffer;
    glm::mat4 m_transform;
    glm::mat4 m_translation;
    glm::mat4 m_size;
    bool m_visible = true;
};

class OGRect : public OGElement {
public:
    void draw(VkCommandBuffer& commandBuffer, uint32_t frame) override;

    void destroy() override;

    void update(float delta) override;

    void create(const glm::vec2 &origin, const glm::vec2 &size);

    bool handleInput(const glm::vec2& touchPosition, bool pressed) override;

protected:
    uint32_t m_textureIndex;
    uint32_t m_objectIndex;
};


#endif //OXYOUS_2026_OGELEMENT_HPP
