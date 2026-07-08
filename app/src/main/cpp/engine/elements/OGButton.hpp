//
// Created by Mr Steven J Baldwin on 08/07/2026.
//

#ifndef OXYOUS_2026_OGBUTTON_HPP
#define OXYOUS_2026_OGBUTTON_HPP

#include "OGElement.hpp"

class OGButton : public OGElement {
public:
    void draw(VkCommandBuffer& commandBuffer, uint32_t frame) override;

    void destroy() override;

    void update(float delta) override;

    void create(const glm::vec2 &origin, const glm::vec2 &size, const std::string &text);

    bool handleInput(const glm::vec2& touchPosition, bool pressed) override;

    void setOnTap(std::function<void()> onTap);

protected:
    std::string m_text;
    glm::vec2 m_position;
    glm::vec2 m_size;
    uint32_t m_textureIndex;
    uint32_t m_objectIndex;
    std::function<void()> m_onTap;
};

#endif //OXYOUS_2026_OGBUTTON_HPP
