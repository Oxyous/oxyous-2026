//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#include "OGPlayerActor.hpp"
#include "engine/Engine.hpp"

OGPlayerActor::OGPlayerActor() {
    m_yaw = 0.0f;
    m_pitch = 0.0f;
    m_distance = 4.0f;
    m_sensitivity = 2.25f;
}

void OGPlayerActor::update(double deltaTime) {
    OGActor::update(deltaTime);

    if (ENGINE->isGameModeFly())
        return;

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

