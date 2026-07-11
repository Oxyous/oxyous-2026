//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "Engine.hpp"
#include "../render/vulkan/DescriptorCache.hpp"
#include "GameView.hpp"
#include "../engine/collision/CollisionHelper.hpp"
#include "engine/components/OGCollisionComponent.hpp"
#include "engine/physics/OGPhysicsManager.hpp"

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
    m_camera->update(deltaTime);

    if (isGameModeFly()) {
        auto camBound = m_camera->getComponent<OGCollisionComponent>();
        auto bounds = camBound->getCollisionVolume<SphereVolume>();

        glm::vec3 camPos = m_camera->getTranslation();

        for (auto &p: GAME_VIEW->getWorldPolygons()) {
            OGContact contact;

            if (CollisionHelper::resolvePolygonSphereCollision(p, *bounds, contact)) {
                // Handle collision response here
                camPos += contact.normal * contact.depth;
                m_camera->setTranslation(camPos);
                //bounds->setCenter(camPos);
            }
        }
    } else {
        /*const auto player = dynamic_cast<OGPlayerActor*>(GAME_VIEW->getActivePlayer().get());
        const auto playerCollision = player->getComponent<OGCollisionComponent>()->getCollisionVolume<CapsuleVolume>();

        glm::vec3 playerPos = player->getTranslation();
        player->setGrounded(false, 0.0f);

        for (auto &p : GAME_VIEW->getWorldPolygons()) {
            OGContact contact;

            if (CollisionHelper::resolvePolygonCapsuleCollision(p, playerCollision->transform(playerPos, player->getRotation()), contact)) {
                playerPos += contact.normal * contact.depth;
                player->setTranslation(playerPos);
            }
        }*/

        const auto player = dynamic_cast<OGPlayerActor*>(GAME_VIEW->getActivePlayer().get());
        const auto playerCollision = player->getComponent<OGCollisionComponent>()->getCollisionVolume<OBBVolume>();

        glm::vec3 playerPos = player->getTranslation();
        player->setGrounded(false, 0.0f);

        for (auto &p : GAME_VIEW->getWorldPolygons()) {
            OGContact contact;

            if (CollisionHelper::resolvePolygonObbCollision(p, *playerCollision, contact)) {
                playerPos += contact.normal * contact.depth;
                player->setTranslation(playerPos);
            }
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
