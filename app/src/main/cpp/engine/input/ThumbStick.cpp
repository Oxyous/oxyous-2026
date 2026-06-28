//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#include "ThumbStick.hpp"
#include "../elements/OGElement.hpp"
#include "../ui/OGUi.hpp"

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

    /* Prepare UI */
    auto innerRect = new OGRect();
    innerRect->create(glm::vec2(0.0f, 0.0f), glm::vec2(192.0f, 192.0f));
    m_innerStick = reinterpret_cast<OGElement *>(UI->addElement(innerRect));

    auto outerRect = new OGRect();
    outerRect->create(glm::vec2(0.0f, 0.0f), glm::vec2(192.0f, 192.0f));
    m_outerStick = reinterpret_cast<OGElement *>(UI->addElement(outerRect));

    return true;
}

bool ThumbStick::isPressed(const glm::vec2 &point) {

    if (!m_isPressed) {
        m_innerStick->setTranslation(glm::vec3(point, 0.0f));
        m_outerStick->setTranslation(glm::vec3(point, 0.0f));
        m_inputPosition = point;
        m_isPressed = true;
    }

    return false;
}

void ThumbStick::onTouchMove(const glm::vec2 &point) {
    if(m_isPressed) {
        setActuator(point);
    }
}

void ThumbStick::onTouchUp(const glm::vec3 point) {
    if (m_isPressed) {
        m_isPressed = false;
        resetActuator();
    }
}

void ThumbStick::onTouchDown(const glm::vec2 &point) {
    if(!m_isPressed) {
        m_isPressed = true;
    }
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
    m_actuator = glm::vec2(0.0f);
}

void ThumbStick::setActuator(const glm::vec2 &point) {
    double deltaX = point.x - m_outerCenter.x;
    double deltaY = point.y - m_outerCenter.y;
    double distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));
    if(distance <= m_outerRadius) {
        m_actuator.x = deltaX / m_outerRadius;
        m_actuator.y = deltaY / m_outerRadius;
    }
    else {
        m_actuator.x = deltaX / distance;
        m_actuator.y = deltaY / distance;
    }
}
