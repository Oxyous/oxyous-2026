//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#ifndef OXYOUS_2026_OGPLAYERACTOR_HPP
#define OXYOUS_2026_OGPLAYERACTOR_HPP


#include "OGActor.hpp"
#include "engine/animation/OGAnimController.hpp"

class OGPlayerActor : public OGDynamicActor {
public:
    OGPlayerActor();
    virtual ~OGPlayerActor() = default;

    virtual void update(double deltaTime) override;

    virtual bool initialize() override;

    /** */
    glm::mat4 getViewMatrix() const;

    /** */
    glm::mat4 getProjectionMatrix() const;

    void jump();

    /** Set Projection Matrix */
    void setProjectionMatrix(const glm::mat4& matrix);

    /** Get Camera Position*/
    glm::vec3 getCameraPosition() const;

    /** Set Is Grounded */
    void setGrounded(bool isGrounded, float groundHeight);

    /** */
    void handleInput(double deltaTime);

private:
    OGAnimController m_animationController;

    float m_moveSpeed = 2.8f;
    float m_movement = 0.0f;
    float m_yaw; // horizontal rotation
    float m_pitch; // vertical rotation
    float m_distance;
    float m_sensitivity; //
    float m_groundHeight = 0.15f;
    float m_targetGroundHeight = 0.0f;
    glm::mat4 m_viewMatrix{};
    glm::mat4 m_projectionMatrix{};
    glm::vec3 m_cameraPosition{};
    bool m_isGrounded;
    bool m_isJumping;

    float m_airTime = 0.0f;

    float m_idleTime = 0.0f;
    float m_runTime = 0.0f;

    std::shared_ptr<OGAnimationClip> m_idleAnimation;
    std::shared_ptr<OGAnimationClip> m_runAnimation;
};

#endif //OXYOUS_2026_OGPLAYERACTOR_HPP
