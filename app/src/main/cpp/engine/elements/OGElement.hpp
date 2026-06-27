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

    virtual glm::mat4 &getTransform() {
        return m_transform;
    }

    virtual bool isVisible() {
        return true;
    }

protected:
    GPUBuffer m_vertexBuffer;
    glm::mat4 m_transform;
};

class OGRect : public OGElement {
public:
    void draw(VkCommandBuffer& commandBuffer, uint32_t frame) override;

    void destroy() override;

    void update(float delta) override;

    void create(const glm::vec2 &origin, const glm::vec2 &size);

protected:
};


#endif //OXYOUS_2026_OGELEMENT_HPP
