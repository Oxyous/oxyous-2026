//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#include "OGPhysicsComponent.hpp"
#include "OGCollisionComponent.hpp"

void OGPhysicsComponent::computeInertia() {
    float ix, iy, iz;

    const auto volume = m_owner->getComponent<OGCollisionComponent>()->getCollisionVolume<IVolume>();
    if (!volume) {
        throw std::runtime_error("Error: Physics Component has no volume");
    }

    if(m_massInverse == 0.0f) {
        m_inverseInertia = glm::mat3(0.0f);
        m_inverseInertiaWorld = glm::mat3(0.0f);
        return;
    }

    const auto obb = dynamic_cast<OBBVolume *>(volume);

    if (obb) {
        float fraction = 1.0f / 3.0f; // Correct for half-extents
        float x2 = obb->getExtents().x * obb->getExtents().x;
        float y2 = obb->getExtents().y * obb->getExtents().y;
        float z2 = obb->getExtents().z * obb->getExtents().z;
        ix = (y2 + z2) * (1.0f / m_massInverse) * fraction;
        iy = (x2 + z2) * (1.0f / m_massInverse) * fraction;
        iz = (x2 + y2) * (1.0f / m_massInverse) * fraction;

        m_inverseInertia = glm::mat3(
                1.0f / ix, 0.0, 0.0,
                0.0, 1.0f / iy, 0.0,
                0.0, 0.0, 1.0f / iz);

        glm::mat3 rot = glm::mat3_cast(m_owner->getRotation());
        m_inverseInertiaWorld = rot * m_inverseInertia * glm::transpose(rot);
        return;
    }

    const auto sphere = dynamic_cast<SphereVolume *>(volume);
    if (sphere) {
        float r2 = sphere->getRadius() * sphere->getRadius();
        float i = (2.0f / 5.0f) * (1.0f / m_massInverse) * r2;
        m_inverseInertia = glm::mat3(1.0f / i);
        m_inverseInertiaWorld = m_inverseInertia;
        return;
    }

    const auto capsule = dynamic_cast<CapsuleVolume *>(volume);
    if (capsule) {
        float r = capsule->getRadius();
        float h = glm::distance(capsule->getBase(), capsule->getTop());
        float mass = 1.0f / m_massInverse;

        // Approximate inertia for a capsule aligned with Y-axis
        // Cylinder + 2 Hemispheres
        float r2 = r * r;
        float h2 = h * h;

        // Moments of inertia
        iy = 0.5f * mass * r2; // Longitudinal axis
        ix = iz = mass * ( (1.0f/12.0f) * h2 + (1.0f/4.0f) * r2 ); // Transverse axes

        m_inverseInertia = glm::mat3(
                1.0f / ix, 0.0, 0.0,
                0.0, 1.0f / iy, 0.0,
                0.0, 0.0, 1.0f / iz);

        glm::mat3 rot = glm::mat3_cast(m_owner->getRotation());
        m_inverseInertiaWorld = rot * m_inverseInertia * glm::transpose(rot);
        return;
    }
}

void OGPhysicsComponent::integrateForces(float dt) {
    if (!m_isAwake || m_massInverse == 0.0f) return;

    // Standard damping (per second)
    m_velocity *= powf(0.98f, dt);
    m_angularVelocity *= powf(0.98f, dt);

    m_velocity += m_acceleration * dt;

    glm::vec3 angularAcceleration = m_inverseInertiaWorld * m_torques;
    m_torques = glm::vec3(0.0f);
    m_angularVelocity += angularAcceleration * dt;
}

void OGPhysicsComponent::integrateVelocity(float dt) {
    if (!m_isAwake || m_massInverse == 0.0f) return;

    // Clamp velocity to prevent tunneling at extreme speeds
    const float maxVelocity = 50.0f;
    if (glm::length(m_velocity) > maxVelocity) {
        m_velocity = glm::normalize(m_velocity) * maxVelocity;
    }

    // Update position based on new velocity
    glm::vec3 newPosition = m_owner->getTranslation() + m_velocity * dt;
    m_owner->setTranslation(newPosition);

    // Update rotation based on new angular velocity
    glm::vec3 scaledAV = m_angularVelocity * dt;
    float angle = glm::length(scaledAV);
    if (angle > 1e-6f) {
        glm::quat deltaRotation = glm::angleAxis(angle, scaledAV / angle);
        m_owner->setRotation(glm::normalize(deltaRotation * m_owner->getRotation()));
    }

    // Update world inertia for the next step/frame
    computeInertia();
}

void OGPhysicsComponent::update(double delta) {
    computeInertia();

    float currentMotion = glm::dot(m_velocity, m_velocity) + glm::dot(m_angularVelocity, m_angularVelocity);
    float bias = powf(0.1f, delta);
    m_motion = bias * m_motion + (1.0f - bias) * currentMotion;

    if (m_motion <= 0.1) {
        m_velocity = glm::vec3(0.0f);
        m_angularVelocity = glm::vec3(0.0f);
        setAwake(false);
    }
}

float OGPhysicsComponent::getMass() {
    return m_massInverse;
}

glm::mat3 OGPhysicsComponent::getInertiaTensor() const {
    return m_inverseInertiaWorld;
}

glm::vec3 OGPhysicsComponent::getVelocity() const {
    return m_velocity;
}

glm::vec3 OGPhysicsComponent::getAngularVelocity() const {
    return m_angularVelocity;
}

float OGPhysicsComponent::getRestitution() {
    return m_restitution;
}

void OGPhysicsComponent::setVelocity(const glm::vec3 &velocity) {
    m_velocity = velocity;
}

void OGPhysicsComponent::setAngularVelocity(const glm::vec3 &angularVelocity) {
    m_angularVelocity = angularVelocity;
}

float OGPhysicsComponent::getFriction() {
    return m_friction;
}

void OGPhysicsComponent::setFriction(float friction) {
    m_friction = friction;
}

void OGPhysicsComponent::setAwake(bool awake) {
    m_isAwake = awake;

}

void OGPhysicsComponent::initialize() {
    if (m_massInverse > 0.0f) {
        auto col = m_owner ? m_owner->getComponent<OGCollisionComponent>() : nullptr;
        if (col && col->getCollisionVolume<IVolume>()) {
            computeInertia();
        }
    }
}

void OGPhysicsComponent::destroy() {

}

void OGPhysicsComponent::render(VkCommandBuffer &commandBuffer, uint64_t currentFrame) {

}

OGEntity *OGPhysicsComponent::getOwner() const {
    return OGComponent::getOwner();
}

void OGPhysicsComponent::setMass(float mass) {

    if (mass == 0.0f) return;
    //m_forces = GRAVITY_CONSTANT * m_massInverse;
    m_massInverse = 1.0f / mass;
}

void OGPhysicsComponent::setAcceleration(const glm::vec3 &acceleration) {
    m_acceleration = acceleration;
}
