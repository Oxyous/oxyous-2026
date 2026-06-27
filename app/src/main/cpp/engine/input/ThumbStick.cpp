//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#include "ThumbStick.hpp"

ThumbStick::ThumbStick(glm::vec2 outerCenter, glm::vec2 innerCenter, float outerRadius,
                       float innerRadius) {
    this->m_outerCenter = outerCenter;
    this->m_innerCenter = innerCenter;
    this->m_outerRadius = outerRadius;
    this->m_innerRadius = innerRadius;
    this->m_isPressed = false;
    this->m_actuator = glm::vec2(0.0f);
}

void ThumbStick::update(float delta) {
    this->m_innerCenter.x = m_outerCenter.x + (m_actuator.x * this->m_outerRadius);
    this->m_innerCenter.y = m_outerCenter.y + (m_actuator.y * this->m_outerRadius);
}

bool ThumbStick::initialize() {
    return false;
}

bool ThumbStick::isPressed(const glm::vec2 &point) {
    return false;
}

void ThumbStick::onTouchMove(const glm::vec2 &point) {

}

void ThumbStick::onTouchUp(const glm::vec3 point) {

}

void ThumbStick::onTouchDown(const glm::vec2 &point) {

}

glm::vec2 ThumbStick::getActuator() {
    return m_actuator;
}

float ThumbStick::getDistance(const glm::vec2 &point) {
    return 0;
}

glm::vec2 ThumbStick::getPosition() {
    return m_inputPosition;
}

void ThumbStick::resetActuator() {

}

void ThumbStick::setActuator(const glm::vec2 &point) {
    m_actuator = point;
}
