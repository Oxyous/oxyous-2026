//
// Created by Mr Steven J Baldwin on 08/07/2026.
//


#include "OGButton.hpp"
#include "ScreenSpaceRenderer.hpp"
#include "engine/ui/OGUi.hpp"

void OGButton::create(const glm::vec2 &origin, const glm::vec2 &size, const std::string &text, const std::string& spriteName) {
    m_position = origin;
    m_size = size;
    m_spriteInstanceId = UI->addSprite(spriteName, origin, size);
}

void OGButton::setOnTap(std::function<void()> onTap) {
    m_onTap = std::move(onTap);
}

bool OGButton::handleInput(const glm::vec2 &touchPos, bool pressed) {
    bool isInside = touchPos.x >= m_position.x && touchPos.x <= m_position.x + m_size.x &&
                    touchPos.y >= m_position.y && touchPos.y <= m_position.y + m_size.y;

    if (isInside && !pressed) {
        if (m_onTap) {
            m_onTap();
        }
        return true;
    }
    return isInside;
}
