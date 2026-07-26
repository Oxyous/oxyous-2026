//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#include "OGCharacter.hpp"
#include "../Engine.hpp"
#include "engine/animation/OGAnimationManager.hpp"
#include "engine/components/OGSkeletalMeshComponent.hpp"
#include "engine/GPUResources.hpp"

bool OGCharacter::initialize() {
    if (!OGActor::initialize()) {
        return false;
    }

    // Load animations if they aren't already loaded by the manager
    ANIMATION_MANAGER->loadAnimation("npc-idle", "animations/player/default.ganim");
    ANIMATION_MANAGER->loadAnimation("npc-walk", "animations/player/run-forward.ganim");

    m_idleAnimation = ANIMATION_MANAGER->getAnimation("npc-idle");
    m_runAnimation = ANIMATION_MANAGER->getAnimation("npc-walk");

    if (m_idleAnimation) {
        m_animationController.playAnimation(m_idleAnimation);
    }

    return true;
}

void OGCharacter::update(double deltaTime) {
    OGActor::update(deltaTime);

    m_animationController.update(deltaTime);

    float movementAmount = 0.0f;

    if(m_path.size() > 0) {
        glm::vec3 targetPosition = m_path[m_pathIndex];
        glm::vec3 direction = targetPosition - getTranslation();
        float distance = glm::length(direction);

        if (distance < 0.1f) {
            m_pathIndex = (m_pathIndex + 1) % m_path.size();
            if (m_pathIndex == 0) {
                setPath(std::vector<glm::vec3>());
            }
        } else {
            direction = glm::normalize(direction);
            setTranslation(getTranslation() + direction * speed * static_cast<float>(deltaTime));

            auto newDirection = glm::quat_cast(lookRotation(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
            m_rotation = glm::lerp(glm::normalize(m_rotation), newDirection, (float) deltaTime * 5.0f);

            movementAmount = speed;
        }
    }

    // Performance Optimization: Skip heavy animation logic if not visible
    if (!ENGINE->isActorVisible(this)) {
        return;
    }

    // Animation sampling and blending
    if (m_idleAnimation && m_runAnimation) {
        m_idleTime += deltaTime;
        m_runTime += deltaTime;

        if (m_idleTime > m_idleAnimation->duration) m_idleTime = fmod(m_idleTime, m_idleAnimation->duration);
        if (m_runTime > m_runAnimation->duration) m_runTime = fmod(m_runTime, m_runAnimation->duration);

        OGAnimationPose idlePose = m_animationController.SampleAnimation(m_idleAnimation, m_idleTime);
        OGAnimationPose runPose = m_animationController.SampleAnimation(m_runAnimation, m_runTime);

        float blendFactor = std::clamp(movementAmount / speed, 0.0f, 1.0f);
        OGAnimationPose blendPose = m_animationController.BlendPose(idlePose, runPose, blendFactor);

        auto skeletalMeshComp = getComponent<OGSkeletalMeshComponent>();
        if (skeletalMeshComp) {
            auto jointInfos = m_idleAnimation->joints; // Assuming hierarchy is same
            std::vector<glm::mat4> globalMatrices = m_animationController.getBlendedGlobalMatrices(jointInfos, blendPose);

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
}

glm::mat4 OGCharacter::lookRotation(glm::vec3 direction, glm::vec3 up) {
    glm::vec3 zAxis = glm::normalize(direction);
    glm::vec3 xAxis = glm::normalize(glm::cross(up, zAxis));
    glm::vec3 yAxis = glm::cross(zAxis, xAxis);

    glm::mat4 rotationMatrix(1.0f);
    rotationMatrix[0] = glm::vec4(xAxis, 0.0f);
    rotationMatrix[1] = glm::vec4(yAxis, 0.0f);
    rotationMatrix[2] = glm::vec4(zAxis, 0.0f);

    return rotationMatrix;
}
