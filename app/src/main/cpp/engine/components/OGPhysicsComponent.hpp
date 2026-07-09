//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#ifndef OXYOUS_2026_OGPHYSICSCOMPONENT_HPP
#define OXYOUS_2026_OGPHYSICSCOMPONENT_HPP


#include "engine/entity/OGEntity.hpp"

class OGPhysicsComponent : public OGComponent {
public:
    void update(double delta) override;

    /** */
    float getMass();

    /** Get Inertia Tensor */
    glm::mat3 getInertiaTensor() const;

    glm::vec3 getVelocity() const;

    void setVelocity(const glm::vec3& velocity);

    glm::vec3 getAngularVelocity() const;

    void setAngularVelocity(const glm::vec3& angularVelocity);

    float getRestitution();

    void setFriction(float friction);

    float getFriction();

    void setAwake(bool awake);

protected:
    void computeInertia();
private:
    float m_massInverse;
    float m_restitution;
    bool m_isAwake;
    float m_friction;
    float m_motion;
    glm::mat3 m_inverseInertia;
    glm::vec3 m_acceleration;
    glm::vec3 m_lastAcceleration;
    glm::vec3 m_velocity;
    glm::vec3 m_angularVelocity;
    glm::vec3 m_position;
    glm::vec3 m_forces;
    glm::vec3 m_torques;
    glm::quat m_orientation;
    glm::vec3 m_lastPosition;
    glm::mat3 m_inverseInertiaWorld;
};


#endif //OXYOUS_2026_OGPHYSICSCOMPONENT_HPP
