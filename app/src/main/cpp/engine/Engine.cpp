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
    m_isExecuting = true;
    return true;
}

void Engine::destroy() {

}

void Engine::render() {

}

void Engine::update(float deltaTime) {
    m_input.update(deltaTime);
    //m_camera->update(deltaTime);

    const float percent = 0.2f;
    if (isGameModeFly()) {
        auto camBound = m_camera->getComponent<OGCollisionComponent>();
        auto bounds = camBound->getCollisionVolume<SphereVolume>();

        glm::vec3 camPos = m_camera->getTranslation();
        std::vector<OGPolygon> polygons;
        GAME_VIEW->getSphereIntersectionByBHV(*bounds, polygons);

        for (auto &p: polygons) {
            OGContact contact;

            if (CollisionHelper::resolvePolygonSphereCollision(p, *bounds, contact)) {
                // Handle collision response here
                camPos += contact.normal * contact.depth * percent;
                m_camera->setTranslation(camPos);
                //bounds->setCenter(camPos);
            }
        }
    } else {
       const auto player = dynamic_cast<OGPlayerActor *>(GAME_VIEW->getActivePlayer().get());
        const auto playerCollision = player->getComponent<OGCollisionComponent>()->getCollisionVolume<CapsuleVolume>();
        glm::vec3 playerPos = player->getTranslation();
        player->setGrounded(false, 0.0f);

        std::vector<OGPolygon> polygons;
        GAME_VIEW->getCapsuleIntersectionByBHV(*playerCollision, polygons);

        for (auto &p: polygons) {
            OGContact contact;

            if (CollisionHelper::resolvePolygonCapsuleCollision(p, *playerCollision, contact)) {
                playerPos += contact.normal * contact.depth * percent;
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

JNIEnv *Engine::getJniEnv() {
    JNIEnv *env;
    int status = m_app->activity->vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0) {
        status = m_app->activity->vm->AttachCurrentThread(&env, nullptr);
        if (status < 0) {
            return nullptr;
        }
    }
    return env;
}
