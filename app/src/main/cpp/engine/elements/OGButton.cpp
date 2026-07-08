//
// Created by Mr Steven J Baldwin on 08/07/2026.
//


#include "OGButton.hpp"
#include "ScreenSpaceRenderer.hpp"
#include "engine/ui/OGUi.hpp"

void OGButton::draw(VkCommandBuffer &commandBuffer, uint32_t frame) {
    if (!isVisible()) return;

    PCScreenElements pc = {};
    pc.transform = getTransform();
    pc.textureIndex = m_textureIndex;
    pc.objectIndex = m_objectIndex;
    vkCmdPushConstants(commandBuffer, SCREEN_RENDER->getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(PCScreenElements), &pc);

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer.buffer, &m_vertexBuffer.offset);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);

    glm::vec2 textPos = m_position + m_size * 0.5f;
    UI->drawString(commandBuffer, m_text, textPos.x, textPos.y, 1.0f);
}

void OGButton::destroy() {

}

void OGButton::update(float delta) {
}

void OGButton::create(const glm::vec2 &origin, const glm::vec2 &size, const std::string &text) {
    m_position = origin;
    m_size = size;
    m_text = text;
    m_visible = true;

    m_vertexBuffer = SCREEN_RENDER->createQuadBuffer(origin, size);
}

void OGButton::setOnTap(std::function<void()> onTap) {
    m_onTap = std::move(onTap);
}

bool OGButton::handleInput(const glm::vec2 &touchPos, bool pressed) {
    if (!isVisible()) return false;

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
