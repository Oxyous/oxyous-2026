//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "Engine.hpp"
#include "../render/vulkan/DescriptorCache.hpp"
#include "GameView.hpp"
#include "../engine/collision/CollisionHelper.hpp"
#include "engine/components/OGCollisionComponent.hpp"
#include "engine/components/OGStaticMeshComponent.hpp"
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
        ENGINE->getSphereIntersectionByBHV(*bounds, polygons);

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
        ENGINE->getCapsuleIntersectionByBHV(*playerCollision, polygons);

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


void Engine::computeCollisionBHV(const std::vector<OGPolygon> &polygons) {
    m_collisionBHV = std::make_unique<BVH>();

    m_collisionBHV->build(polygons, 4);
}

/** Get Possible Polygons capsule collision */
void Engine::getCapsuleIntersectionByBHV(const CapsuleVolume &capsule,
                                         std::vector<OGPolygon> &polygons) {
    m_collisionBHV->intersects(capsule, polygons);
}

void
Engine::getSphereIntersectionByBHV(const SphereVolume &sphere, std::vector<OGPolygon> &polygons) {
    m_collisionBHV->intersects(sphere, polygons);
}

void Engine::getObbIntersectionByBHV(const OBBVolume &obb, std::vector<OGPolygon> &polygons) {
    m_collisionBHV->intersects(obb, polygons);
}

void
Engine::getSegmentIntersectionByBHV(const OGSegment &segment, std::vector<OGPolygon> &polygons) {
    m_collisionBHV->intersects(segment, polygons);
}

void Engine::buildStaticBVH(std::vector<AABBVolume> &primitives) {
    m_staticBVH = std::make_unique<OGBVH<AABBVolume>>();
    m_staticBVH->build(primitives, 4);
}

void Engine::getStaticFrustumIntersectionByBVH(const Frustum &frustum,
                                               std::vector<AABBVolume> &entities) {
    if (m_staticBVH) {
        m_staticBVH->intersectsFrustum(frustum, entities);
    }
}

void
Engine::getStaticIntersectionByBVH(const AABBVolume &volume, std::vector<AABBVolume> &entities) {
    if (m_staticBVH) {
        m_staticBVH->intersects(volume, entities);
    }
}

void Engine::buildLevelOctree() {
    m_staticOctree = std::make_unique<OGOctree<AABBVolume>>();
    std::vector<AABBVolume> volumes;
    GAME_VIEW->buildStaticVolumes(volumes);
    m_staticOctree->build(volumes);
}


void Engine::queryOctreeFrustum(const Frustum& frustum, std::vector<AABBVolume>& volumes)
{
    if (m_staticOctree) {
        m_staticOctree->query(frustum, volumes);
    }
}

void Engine::queryOctree(const AABBVolume& volume, std::vector<AABBVolume>& volumes)
{
    if (m_staticOctree) {
        m_staticOctree->query(volume, volumes);
    }
}

void Engine::updateVisibleObjects() {
    m_visibleObjects.clear();
    const auto frustum = getCameraFrustum();

    std::vector<AABBVolume> visibleVolumes;
    queryOctreeFrustum(frustum, visibleVolumes);

    // Fallback to BVH if octree query returned nothing, so culling never blanks the whole scene.
    if (visibleVolumes.empty()) {
        getStaticFrustumIntersectionByBVH(frustum, visibleVolumes);
    }

    m_visibleObjects.reserve(visibleVolumes.size());
    for (const auto &vol: visibleVolumes) {
        if (auto *owner = vol.getOwner()) {
            m_visibleObjects.push_back(owner);
        }
    }

    // Final fallback: keep rendering mesh entities if culling structures return no results.
    if (m_visibleObjects.empty()) {
        const auto &entities = GAME_VIEW->getEntities();
        m_visibleObjects.reserve(entities.size());
        for (const auto &[name, entity]: entities) {
            if (entity && entity->hasComponent<OGStaticMeshComponent>()) {
                m_visibleObjects.push_back(entity.get());
            }
        }
    }
}

Frustum Engine::getCameraFrustum() {
    Frustum frustum;
    if (ENGINE->isDemoCulling()) {
        frustum.update(ENGINE->getFlyingCameraProjection(), ENGINE->getFlyingCameraView());
    } else {
        frustum.update(ENGINE->getCameraProjection(), ENGINE->getCameraView());
    }
    return frustum;
}
