//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "Engine.hpp"
#include "../render/vulkan/DescriptorCache.hpp"
#include "GameView.hpp"
#include "../engine/collision/CollisionHelper.hpp"

bool Engine::initialize(android_app *app) {
    m_app = app;

    return true;
}

void Engine::destroy() {

}

void Engine::render() {

}

void Engine::update(float deltaTime) {
    m_input.update(deltaTime);
    m_camera.update(deltaTime);

    auto camBound = m_camera.getBounds();

    glm::vec3 camPos = m_camera.getTranslation();

    for (auto &p: GAME_VIEW->getWorldPolygons()) {
        OGContact contact;

        if (CollisionHelper::resolvePolygonSphereCollision(p, camBound, contact)) {
            // Handle collision response here
            camPos += contact.normal * contact.depth;
            m_camera.setTranslation(camPos);
            camBound.setCenter(camPos);
        }
    }
}

void Engine::handleInput() {
    m_input.handleInput();
}

void Engine::prepareInput() {
    m_input.initialize();
}

ThumbStick *Engine::getThumbStick(ThumbStickType type) {
    return m_input.getThumbStick(type);
}

bool Engine::postInitialize() {
    return true;
}

glm::mat4 Engine::preRotation() {
    return glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}
