//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#ifndef OXYOUS_2026_OGPHYSICSCOMPONENT_HPP
#define OXYOUS_2026_OGPHYSICSCOMPONENT_HPP


#define GRAVITY_CONSTANT glm::vec3(0.0, -9.0, 0.0)

#include "engine/entity/OGEntity.hpp"

class OGPhysicsComponent : public OGComponent {
public:

    GET_UNIQUE_TYPE(OGPhysicsComponent)

    OGPhysicsComponent() {
        m_inverseInertia = glm::mat3(1.0f);
        m_inverseInertiaWorld = glm::mat3(1.0f);
        m_acceleration = GRAVITY_CONSTANT;
        m_lastAcceleration  = GRAVITY_CONSTANT;
    }

public:
    void update(double delta) override;

    void integrateForces(float dt);

    void integrateVelocity(float dt);

    /** */
    float getMass();

    float getInverseMass() const { return m_massInverse; }

    glm::mat3 getInverseInertiaWorld() const { return m_inverseInertiaWorld; }

    void setMass(float mass);

    /** Get Inertia Tensor */
    glm::mat3 getInertiaTensor() const;

    glm::vec3 getVelocity() const;

    void setVelocity(const glm::vec3& velocity);

    glm::vec3 getAngularVelocity() const;

    void setAngularVelocity(const glm::vec3& angularVelocity);

    void setAcceleration(const glm::vec3& acceleration);

    float getRestitution();

    void setRestitution(float restitution) {
        m_restitution = restitution;
    }

    void initialize() override;

    void destroy() override;

    void render(VkCommandBuffer &commandBuffer, uint64_t currentFrame) override;

    OGEntity *getOwner() const override;

    void setFriction(float friction);

    float getFriction();

    void setAwake(bool awake);

    void setMotion(float motion);

    /** Set Rotation Lock */
    void setRotationLock(bool lockX, bool lockY, bool lockZ) {
        m_lockRotationX = lockX;
        m_lockRotationY = lockY;
        m_lockRotationZ = lockZ;
    }

    bool isAwake() const { return m_isAwake; }

    void setGrounded(bool grounded) { m_isGrounded = grounded; }
    bool isGrounded() const { return m_isGrounded; }

protected:
    void computeInertia();
private:
    float m_massInverse = 0.0f;
    float m_restitution = 0.1f;
    bool m_isAwake = true;
    float m_friction = 1.0f;
    float m_motion = 1.0f;
    bool m_isGrounded = false;
    bool m_lockRotationX = false;
    bool m_lockRotationY = false;
    bool m_lockRotationZ = false;
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
