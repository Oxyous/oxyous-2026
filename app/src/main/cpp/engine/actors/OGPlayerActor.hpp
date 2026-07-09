//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#ifndef OXYOUS_2026_OGPLAYERACTOR_HPP
#define OXYOUS_2026_OGPLAYERACTOR_HPP


#include "OGActor.hpp"

class OGPlayerActor : public OGActor {
public:
    OGPlayerActor();
    virtual ~OGPlayerActor() = default;

    virtual void update(double deltaTime) override;

    virtual bool initialize() override;

    /** */
    glm::mat4 getViewMatrix() const;

    /** */
    glm::mat4 getProjectionMatrix() const;

    /** Set Projection Matrix */
    void setProjectionMatrix(const glm::mat4& matrix);

    /** Get Camera Position*/
    glm::vec3 getCameraPosition() const;

    /** Set Is Grounded */
    void setGrounded(bool isGrounded, float groundHeight);

private:
    float m_moveSpeed = 1.0f;
    float m_yaw; // horizontal rotation
    float m_pitch; // vertical rotation
    float m_distance;
    float m_sensitivity; //
    glm::mat4 m_viewMatrix{};
    glm::mat4 m_projectionMatrix{};
    glm::vec3 m_cameraPosition{};
    bool m_isGrounded;
};

#endif //OXYOUS_2026_OGPLAYERACTOR_HPP
