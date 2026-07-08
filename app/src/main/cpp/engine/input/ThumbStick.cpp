//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#include "ThumbStick.hpp"
#include "../elements/OGElement.hpp"
#include "../ui/OGUi.hpp"

ThumbStick::ThumbStick(ThumbStickType type, float outerRadius, float innerRadius) {
    this->m_type = type;
    this->m_outerRadius = outerRadius;
    this->m_innerRadius = innerRadius;
    this->m_isPressed = false;
    this->m_actuator = glm::vec2(0.0f);
}

void ThumbStick::update(double delta) {
    this->m_innerCenter.x = m_outerCenter.x + (m_actuator.x * this->m_outerRadius);
    this->m_innerCenter.y = m_outerCenter.y + (m_actuator.y * this->m_outerRadius);

    if (m_outerStick) {
        m_outerStick->setTranslation(glm::vec3(m_outerCenter, 0.0f));
    }
    if (m_innerStick) {
        m_innerStick->setTranslation(glm::vec3(m_innerCenter, 0.0f));
    }
}

bool ThumbStick::initialize() {

    /* Prepare UI */
    auto outerRect = new OGRect();
    outerRect->setVisible(false);
    outerRect->create(glm::vec2(0.0f, 0.0f), glm::vec2(256.0f, 256.0f));
    m_outerStick = reinterpret_cast<OGElement *>(UI->addElement(outerRect));

    auto innerRect = new OGRect();
    innerRect->setVisible(false);
    innerRect->create(glm::vec2(0.0f, 0.0f), glm::vec2(192.0f, 192.0f));
    m_innerStick = reinterpret_cast<OGElement *>(UI->addElement(innerRect));

    return true;
}

bool ThumbStick::isPressed() {
    return m_isPressed;
}

void ThumbStick::onTouchMove(const glm::vec2 &point) {
    if(m_isPressed) {
        setActuator(point);
    }
}

void ThumbStick::onTouchUp(const glm::vec2& point) {
    if (m_isPressed) {
        m_isPressed = false;
        resetActuator();
        m_outerStick->setVisible(false);
        m_innerStick->setVisible(false);
    }
}

void ThumbStick::onTouchDown(const glm::vec2 &point) {
    if(!m_isPressed) {
        m_isPressed = true;
        m_outerCenter = point;
        m_outerStick->setTranslation(glm::vec3(point, 0.0f));
        m_innerStick->setTranslation(glm::vec3(point, 0.0f));
        m_inputPosition = point;
        m_outerStick->setVisible(true);
        m_innerStick->setVisible(true);
    }
}

glm::vec2 ThumbStick::getActuator() {
    return m_actuator;
}

float ThumbStick::getDistance(const glm::vec2 &point) {
    return glm::length(m_inputPosition - point);
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
