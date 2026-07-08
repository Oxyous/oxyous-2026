//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#include "OGElement.hpp"
#include "../../DataStructures.hpp"
#include "../../render/vulkan/RenderFramework.hpp"
#include "ScreenSpaceRenderer.hpp"
#include "../../resources/GPUTextureResource.hpp"
#include "../../resources/ResourceManager.hpp"

void OGRect::draw(VkCommandBuffer& commandBuffer, uint32_t frame) {

    PCScreenElements pc = {};
    pc.transform = getTransform();
    pc.textureIndex = m_textureIndex;
    pc.objectIndex = m_objectIndex;
    vkCmdPushConstants(commandBuffer, SCREEN_RENDER->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PCScreenElements), &pc);

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer.buffer, &m_vertexBuffer.offset);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void OGRect::destroy() {

}

void OGRect::update(float delta) {
    GPUElementHandle handle{};
    handle.transform = getTransform();
    handle.textureId = m_textureIndex;
    SCREEN_RENDER->updateElement(m_objectIndex, handle);
}

void OGRect::create(const glm::vec2 &origin, const glm::vec2 &size) {

    GPUElementHandle handle{};
    handle.transform = m_transform;
    handle.textureId = m_textureIndex;
    m_objectIndex = SCREEN_RENDER->registerObject(handle);

    OGVertex2D vertices[4] = {};

    // Rect from (0,0) to (size.x, size.y) in local space
    // Top Left
    vertices[0].position = glm::vec2(-1.0f, -1.0f);
    vertices[0].uv = glm::vec2(1.0, 1.0);

    // Top Right
    vertices[1].position = glm::vec2(1.0f, -1.0f);
    vertices[1].uv = glm::vec2(1.0, 0.0);

    // Bottom Right
    vertices[2].position = glm::vec2(1.0f, 1.0f);
    vertices[2].uv = glm::vec2(0.0, 0.0);

    // Bottom Left
    vertices[3].position = glm::vec2(-1.0f, 1.0f);
    vertices[3].uv = glm::vec2(0.0, 1.0);

    if (!RenderFramework::createStagingBuffer(vertices, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &m_vertexBuffer)) {
        aout << "Failed to create vertex buffer!" << std::endl;
        return;
    }

    m_size = glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

    m_textureIndex = SCREEN_RENDER->registerTexture(*RESOURCE_MANAGER->get<GPUTextureResource>("thumbstick-bkg.png")->get());
}

bool OGRect::handleInput(const glm::vec2 &touchPosition, bool pressed) {
    return true;
}
