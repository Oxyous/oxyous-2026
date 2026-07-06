//
// Created by Mr Steven J Baldwin on 29/06/2026.
//

#include "OGCamera.hpp"
#include "../Engine.hpp"

OGCamera::~OGCamera() {

}

void OGCamera::update(double delta) {
    m_position = getTranslation();
    m_yaw -= ENGINE->getThumbStick(THUMBSTICK_RIGHT)->getActuator().x * m_sensitivity;
    m_pitch -= ENGINE->getThumbStick(THUMBSTICK_RIGHT)->getActuator().y * m_sensitivity;

    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;

    m_forward.x = cosf(glm::radians(m_yaw)) * cosf(glm::radians(m_pitch));
    m_forward.y = sinf(glm::radians(m_pitch));
    m_forward.z = sinf(glm::radians(m_yaw)) * cosf(glm::radians(m_pitch));
    m_forward = glm::normalize(m_forward);

    m_position -= m_forward * ENGINE->getThumbStick(THUMBSTICK_LEFT)->getActuator().y * m_speed * (float)delta;
    m_position -= glm::normalize(glm::cross(m_forward, m_up)) * ENGINE->getThumbStick(THUMBSTICK_LEFT)->getActuator().x * m_speed * (float)delta;
    setTranslation(m_position);

    m_bounds.setCenter(m_position);

    m_viewMatrix = glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 OGCamera::getViewMatrix() {
    return m_viewMatrix;
}
glm::vec3 OGCamera::getPosition(){
    return m_position;
}

glm::mat4 OGCamera::getProjectionMatrix() {
    return m_projection;
}

void OGCamera::setProjectionMatrix(glm::mat4 projection) {
    m_projection = projection;
}
