//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#ifndef OXYOUS_2026_THUMBSTICK_HPP
#define OXYOUS_2026_THUMBSTICK_HPP


#include "../../includes.hpp"

enum ThumbStickType {
    THUMBSTICK_LEFT,
    THUMBSTICK_RIGHT
};

class OGElement;

class ThumbStick {
public:
    ThumbStick(ThumbStickType type, float outerRadius, float innerRadius);
public:
    /* */
    virtual void update(double delta);

    /* */
    virtual bool initialize();

    /* */
    virtual bool isPressed();

    /* */
    virtual void onTouchMove(const glm::vec2& point);

    /* */
    virtual void onTouchUp(const glm::vec2& point);

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

    ThumbStickType getType() {
        return m_type;
    }

private:
    /* */
    void setActuator(const glm::vec2& point);

    OGElement* m_innerStick;
    OGElement* m_outerStick;
    int m_pointerId = -1;
protected:
    glm::vec2 m_outerCenter;
    glm::vec2 m_innerCenter;
    float m_outerRadius;
    float m_innerRadius;
    glm::vec2 m_actuator;
    bool m_isPressed;
    glm::vec2 m_inputPosition;
    ThumbStickType m_type;
};


#endif //OXYOUS_2026_THUMBSTICK_HPP
