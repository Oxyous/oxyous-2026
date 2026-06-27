//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#include "OGElement.hpp"
#include "../../DataStructures.hpp"
#include "../../render/vulkan/RenderFramework.hpp"

void OGRect::draw(VkCommandBuffer& commandBuffer, uint32_t frame) {
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer.buffer, &m_vertexBuffer.offset);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void OGRect::destroy() {

}

void OGRect::update(float delta) {

}

void OGRect::create(const glm::vec2 &origin, const glm::vec2 &size) {
    OGVertex2D vertices[4] = {};

    // Top Left
    vertices[0].position = glm::vec2(0.0f - size.x, 0.0f + size.y);
    vertices[0].uv = glm::vec2(0.0, 1.0);

    // Top Right
    vertices[1].position = glm::vec2(0.0f + size.x, 0.0f + size.y);
    vertices[1].uv = glm::vec2(1.0, 1.0);

    // Bottom Right
    vertices[2].position = glm::vec2(0.0f + size.x, 0.0f - size.y);
    vertices[2].uv = glm::vec2(1.0, 0.0);

    // Bottom Left
    vertices[3].position = glm::vec2(0.0f - size.x, 0.0f - size.y);
    vertices[3].uv = glm::vec2(0.0, 0.0);

    if (!RenderFramework::createStagingBuffer(vertices, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &m_vertexBuffer)) {
        aout << "Failed to create vertex buffer!" << std::endl;
        return;
    }

    m_transform = glm::translate(glm::mat4(1.0f), glm::vec3(origin, 0.0f));

}