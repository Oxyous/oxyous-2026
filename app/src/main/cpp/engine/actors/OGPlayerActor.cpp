//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#include "OGPlayerActor.hpp"
#include "../Engine.hpp"
#include "engine/components/OGCollisionComponent.hpp"
#include "engine/components/OGPhysicsComponent.hpp"
#include "engine/components/OGSkeletalMeshComponent.hpp"
#include "engine/animation/OGAnimationManager.hpp"
#include "engine/GPUResources.hpp"
#include "engine/collision/CollisionHelper.hpp"
#include "engine/GameView.hpp"
#include "system/OGTimer.hpp"

OGPlayerActor::OGPlayerActor() {
    m_yaw = 0.0f;
    m_pitch = 0.0f;
    m_distance = 3.0f;
    m_sensitivity = 4.25f;
}

void OGPlayerActor::update(double deltaTime) {
    OGActor::update(deltaTime);

    m_animationController.update(deltaTime);

    m_idleTime += deltaTime;
    m_runTime += deltaTime;

    if (m_idleTime > m_idleAnimation->duration) {
        fmod(m_idleTime, m_idleAnimation->duration);
    }

    if (m_runTime > m_runAnimation->duration) {
        fmod(m_runTime, m_runAnimation->duration);
    }

    OGAnimationPose idlePose = m_animationController.SampleAnimation(m_idleAnimation, m_idleTime);
    OGAnimationPose runPose = m_animationController.SampleAnimation(m_runAnimation, m_runTime);

    float blendFactor = m_movement / m_moveSpeed;

    blendFactor = std::clamp(blendFactor, 0.0f, 1.0f);

    OGAnimationPose blendPose = m_animationController.BlendPose(idlePose, runPose, blendFactor);

    // Performance Optimization: Skip heavy animation logic if not visible
    if (!ENGINE->isActorVisible(this)) {
        return;
    }

    auto skeletalMeshComp = getComponent<OGSkeletalMeshComponent>();
    if (skeletalMeshComp) {
        // We need the hierarchy from the current animation clip
        auto clip = &blendPose;
        auto jointInfos = m_animationController.getCurrentAnimation()->joints;
        if (clip) {
            std::vector<glm::mat4> globalMatrices = m_animationController.getBlendedGlobalMatrices(jointInfos, *clip);

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

    // Grounded detection and stabilization
    const auto &collision = getComponent<OGCollisionComponent>();
    auto physics = getComponent<OGPhysicsComponent>();

    if (collision && physics && !m_isJumping) {
        const auto volume = collision->getCollisionVolume<CapsuleVolume>();
        if (volume) {
            float radius = volume->getRadius();
            // volume->getBase() is the center of the bottom sphere.
            // Probe from slightly above the bottom of the capsule to slightly below.
            const auto segment = OGSegment{volume->getBase() - glm::vec3(0, radius - 0.1f, 0),
                                           volume->getBase() - glm::vec3(0, radius + 0.2f, 0)};
            std::vector<OGPolygon> polygons;
            ENGINE->getSegmentIntersectionByBHV(segment, polygons);

            bool foundGround = false;
            float highestGroundHeight = -FLT_MAX;

            for (const auto& poly : polygons) {
                const auto &plane = CollisionHelper::getPolygonPlane(poly);

                // Only consider upward-facing surfaces (floors)
                if (plane.m_normal.y > 0.6f) {
                    // distanceToGround is distance from volume->getBase() (center) to the plane
                    const float distanceToGround =
                            glm::dot(plane.m_normal, volume->getBase()) - plane.m_distance;

                    // The distance from center to floor should be radius.
                    // Allow for a small margin.
                    if (distanceToGround < (radius + 0.15f)) {
                        // Floor height = base.y - distanceToGround
                        // Target translation Y = Floor height
                        float targetPivotY = volume->getBase().y - distanceToGround;

                        if (targetPivotY > highestGroundHeight) {
                            highestGroundHeight = targetPivotY;
                            foundGround = true;
                        }
                    }
                }
            }

            if (foundGround) {
                setGrounded(true, highestGroundHeight);
                physics->setGrounded(true);
                m_airTime = 0.0f;
            } else {
                m_airTime += static_cast<float>(deltaTime);
                // Only lose grounded status after being in the air for a short duration
                // OR if moving upward (jumping)
                if (m_airTime > 0.1f || physics->getVelocity().y > 0.1f) {
                    setGrounded(false, 0.0f);
                    physics->setGrounded(false);
                }
            }
        }
    }
}

bool OGPlayerActor::initialize() {
    if (!OGActor::initialize()) {
        return false;
    }
    ANIMATION_MANAGER->loadAnimation("default-player", "animations/player2/idle-anim.ganim");
    ANIMATION_MANAGER->loadAnimation("player-run", "animations/player2/run-anim.ganim");

    m_animationController.playAnimation(ANIMATION_MANAGER->getAnimation("default-player"));
    m_animationController.playAnimation(ANIMATION_MANAGER->getAnimation("player-run"));

    m_idleAnimation = ANIMATION_MANAGER->getAnimation("default-player");
    m_runAnimation = ANIMATION_MANAGER->getAnimation("player-run");

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
    m_isGrounded = isGrounded;
    m_targetGroundHeight = groundHeight;
}

void OGPlayerActor::handleInput(double deltaTime) {

    if (ENGINE->isGameModeFly()) return;

    m_yaw += ENGINE->getThumbStick(THUMBSTICK_RIGHT)->getActuator().x * m_sensitivity * deltaTime;
    m_pitch -= ENGINE->getThumbStick(THUMBSTICK_RIGHT)->getActuator().y * m_sensitivity * deltaTime;

    m_pitch = glm::clamp(m_pitch, -glm::radians(80.0f), glm::radians(80.0f));

    glm::vec3 forward;
    forward.x = cosf(m_pitch) * sinf(m_yaw);
    forward.y = sinf(m_pitch);
    forward.z = cosf(m_pitch) * cosf(m_yaw);

    forward = glm::normalize(forward);

    glm::vec3 target = this->getTranslation() + glm::vec3(0.0, 1.0f, 0.0);
    m_cameraPosition = target - forward * m_distance;

    glm::vec3 camForward = forward;
    camForward.y = 0;
    camForward = glm::normalize(camForward);

    glm::vec3 cameraRight = glm::normalize(glm::cross(camForward, glm::vec3(0.0, 1.0, 0.0)));

    glm::vec3 moveDir = camForward * -ENGINE->getThumbStick(THUMBSTICK_LEFT)->getActuator().y +
                        cameraRight * -ENGINE->getThumbStick(THUMBSTICK_LEFT)->getActuator().x;

    auto physComp = getComponent<OGPhysicsComponent>();
    if (physComp) {
        glm::vec3 currentVel = physComp->getVelocity();
        glm::vec3 targetVel = moveDir * m_moveSpeed;

        float finalVy = currentVel.y;
        if (m_isGrounded) {
            // Soft Snap: Correct vertical velocity to reach floor height
            float verticalError = m_targetGroundHeight - getTranslation().y;

            // Proportional correction with clamping
            float snapVelocity = verticalError * 10.0f; // Kp = 10
            finalVy = std::clamp(snapVelocity, -5.0f, 5.0f);

            // If the error is tiny, just stop vertical movement
            if (std::abs(verticalError) < 0.001f) {
                finalVy = 0.0f;
            }
        }

        physComp->setVelocity(glm::vec3(targetVel.x, finalVy, targetVel.z));
        physComp->setAwake(true);
    }

    m_movement = glm::length(moveDir * m_moveSpeed);

    if (glm::length(moveDir) > 0.01f) {
        float targetYaw = atan2f(moveDir.x, moveDir.z);
        setRotation(glm::vec3(0.0f, targetYaw, 0.0f));
    }

    m_viewMatrix = glm::lookAt(m_cameraPosition, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void OGPlayerActor::jump() {
    if (m_isGrounded) {
        auto physComp = getComponent<OGPhysicsComponent>();
        if (physComp) {
            physComp->setVelocity(glm::vec3(physComp->getVelocity().x, 2.5f, physComp->getVelocity().z));
            physComp->setAwake(true);
            setGrounded(false, 0.0f);
            m_isJumping = true;

            SYS_TIMER->onTimeoutCallback([this]() {
                m_isJumping = false;
            }, 500);
        }
    }
}

