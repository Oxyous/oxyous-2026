//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#include "OGPlayerActor.hpp"
#include "engine/Engine.hpp"
#include "engine/components/OGCollisionComponent.hpp"
#include "engine/components/OGSkeletalMeshComponent.hpp"
#include "engine/animation/OGAnimationManager.hpp"
#include "engine/GPUResources.hpp"

OGPlayerActor::OGPlayerActor() {
    m_yaw = 0.0f;
    m_pitch = 0.0f;
    m_distance = 2.0f;
    m_sensitivity = 4.25f;
}

void OGPlayerActor::update(double deltaTime) {
    OGActor::update(deltaTime);

    if (ENGINE->isGameModeFly())
        return;

    m_animationController.update(deltaTime);

    auto skeletalMeshComp = getComponent<OGSkeletalMeshComponent>();
    if (skeletalMeshComp) {
        // We need the hierarchy from the current animation clip
        auto clip = m_animationController.getCurrentAnimation();
        if (clip) {
            std::vector<glm::mat4> globalMatrices = m_animationController.getCurrentGlobalMatrices(clip->joints);

            // Multiply by inverse bind pose
            // Note: OGSkeletalMesh stores inverse global bind poses
            auto meshRes = skeletalMeshComp->getMeshResource();
            if (meshRes && meshRes->getSkeletalMesh()) {
                const auto& invBindPose = meshRes->getSkeletalMesh()->getInverseBindPose();
                std::vector<glm::mat4> skinningMatrices(globalMatrices.size());

                for (size_t i = 0; i < globalMatrices.size(); i++) {
                    if (i < invBindPose.size()) {
                        skinningMatrices[i] = globalMatrices[i] * invBindPose[i];
                    } else {
                        skinningMatrices[i] = globalMatrices[i];
                    }
                }

                GPU_RESOURCES->updateBones(skeletalMeshComp->getBoneIndex(), skinningMatrices);
            }
        }
    }



   /* const auto &collision = getComponent<OGCollisionComponent>();
    if (collision) {
        const auto volume = collision->getCollisionVolume<CapsuleVolume>();
        if (volume) {
            const auto segment = OGSegment{volume->getBase() + glm::vec3(0, 0.0f, 0),
                                           volume->getBase() - glm::vec3(0, 0.5f, 0)};
            std::vector<OGPolygon> polygons;
            GAME_VIEW->getSegmentIntersectionByBHV(segment, polygons);
            if (!polygons.empty()) {
                const auto &closestPolygon = polygons[0];
                const auto &plane = CollisionHelper::getPolygonPlane(closestPolygon);
                const float distanceToGround =
                        glm::dot(plane.m_normal, volume->getBase()) - plane.m_distance;

                if (distanceToGround < m_groundHeight) {
                    setGrounded(true, volume->getBase().y - distanceToGround);
                } else {
                    setGrounded(false, 0.0f);
                }
            } else {
                setGrounded(false, 0.0f);
            }
        }
    }
    auto physics = getComponent<OGPhysicsComponent>();
    if (physics) {
        if (!m_isGrounded) {
            // Apply gravity or other physics-related updates here
            physics->setAcceleration(glm::vec3(0.0f, -9.81f, 0.0f)); // Example: Apply gravity
            physics->setMass(1.0f); // Example: Set mass for physics
        }else {
            physics->setAcceleration(glm::vec3(0.0f, 0.0f, 0.0f)); // No acceleration when grounded
            physics->setMass(0.0f); // Example: Set mass for physics
        }
    }*/


    m_yaw += ENGINE->getThumbStick(THUMBSTICK_RIGHT)->getActuator().x * m_sensitivity * deltaTime;
    m_pitch -= ENGINE->getThumbStick(THUMBSTICK_RIGHT)->getActuator().y * m_sensitivity * deltaTime;

    m_pitch = glm::clamp(m_pitch, -glm::radians(80.0f), glm::radians(80.0f));

    glm::vec3 forward;
    forward.x = cosf(m_pitch) * sinf(m_yaw);
    forward.y = sinf(m_pitch);
    forward.z = cosf(m_pitch) * cosf(m_yaw);

    forward = glm::normalize(forward);

    glm::vec3 target = this->getTranslation() + glm::vec3(0.0, 1.8f, 0.0);
    m_cameraPosition = target - forward * m_distance;

    glm::vec3 camForward = forward;
    camForward.y = 0;
    camForward = glm::normalize(camForward);

    glm::vec3 cameraRight = glm::normalize(glm::cross(camForward, glm::vec3(0.0, 1.0, 0.0)));

    glm::vec3 moveDir = camForward * -ENGINE->getThumbStick(THUMBSTICK_LEFT)->getActuator().y +
                        cameraRight * -ENGINE->getThumbStick(THUMBSTICK_LEFT)->getActuator().x;

    setTranslation(getTranslation() + moveDir * m_moveSpeed * static_cast<float>(deltaTime));

    if (glm::length(moveDir) > 0.01f) {
        float targetYaw = atan2f(moveDir.x, moveDir.z);
        setRotation(glm::vec3(0.0f, targetYaw, 0.0f));
    }

    m_viewMatrix = glm::lookAt(m_cameraPosition, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

bool OGPlayerActor::initialize() {
    if (!OGActor::initialize()) {
        return false;
    }
    ANIMATION_MANAGER->loadAnimation("default-player", "animations/player/default.ganim");

    m_animationController.playAnimation(ANIMATION_MANAGER->getAnimation("default-player"));

    return true;
}

glm::mat4 OGPlayerActor::getViewMatrix() const {
    return m_viewMatrix;
}

glm::mat4 OGPlayerActor::getProjectionMatrix() const {
    return m_projectionMatrix;
}

void OGPlayerActor::setProjectionMatrix(const glm::mat4 &matrix) {
    m_projectionMatrix = matrix;
}

glm::vec3 OGPlayerActor::getCameraPosition() const {
    return m_cameraPosition;
}

void OGPlayerActor::setGrounded(bool isGrounded, float groundHeight) {
    if (isGrounded) {
        setTranslation(glm::vec3(getTranslation().x, groundHeight, getTranslation().z));
    }
    m_isGrounded = isGrounded;
}

