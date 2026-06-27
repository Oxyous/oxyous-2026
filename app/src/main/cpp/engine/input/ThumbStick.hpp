//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#ifndef OXYOUS_2026_THUMBSTICK_HPP
#define OXYOUS_2026_THUMBSTICK_HPP


#include "../../includes.hpp"

class OGShape;

class ThumbStick {
public:
    ThumbStick(glm::vec2 outerCenter, glm::vec2 innerCenter, float outerRadius, float innerRadius);
public:
    /* */
    virtual void update(float delta);

    /* */
    virtual bool initialize();

    /* */
    virtual bool isPressed(const glm::vec2& point);

    /* */
    virtual void onTouchMove(const glm::vec2& point);

    /* */
    virtual void onTouchUp(const glm::vec3 point);

    /* */
    virtual void onTouchDown(const glm::vec2& point);

    /* */
    virtual glm::vec2 getActuator();

    /* */
    virtual float getDistance(const glm::vec2& point);

    /* */
    virtual glm::vec2 getPosition();

    /* */
    virtual void resetActuator();

private:
    /* */
    void setActuator(const glm::vec2& point);

    OGShape* m_innerStick;
    OGShape* m_outerStick;
    int m_pointerId = -1;
protected:
    glm::vec2 m_outerCenter;
    glm::vec2 m_innerCenter;
    float m_outerRadius;
    float m_innerRadius;
    glm::vec2 m_actuator;
    bool m_isPressed;
    glm::vec2 m_inputPosition;
};


#endif //OXYOUS_2026_THUMBSTICK_HPP
