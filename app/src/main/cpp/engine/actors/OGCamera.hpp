//
// Created by Mr Steven J Baldwin on 29/06/2026.
//

#ifndef OXYOUS_2026_OGCAMERA_HPP
#define OXYOUS_2026_OGCAMERA_HPP

#include "../../includes.hpp"
#include "OGActor.hpp"

class OGCamera : public OGActor {
public:
    OGCamera() : OGActor() {
        m_position = glm::vec3(0.0f);
        m_up = glm::vec3(0.0f, 1.0f, 0.0f);
        m_forward = glm::vec3(0.0f);
        m_yaw = 0.0f;
        m_pitch = 0.0f;
        m_speed = 8.0f;
        m_sensitivity = 0.33f;
    }

    ~OGCamera();

public:

    void update(double delta) override;

    virtual glm::mat4 getViewMatrix();

protected:
    glm::vec3 m_position{};
    glm::vec3 m_up{};
    glm::vec3 m_forward{};
    float m_yaw;
    float m_pitch;
    float m_speed;
    float m_sensitivity;
    glm::mat4 m_viewMatrix{};
};


#endif //OXYOUS_2026_OGCAMERA_HPP
