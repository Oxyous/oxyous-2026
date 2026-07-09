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

    const auto obb = dynamic_cast<OBBVolume*>(volume);

    if (obb) {
        float fraction = 1.0f / 12.0f;
        float x2 = obb->getExtents().x * obb->getExtents().x;
        float y2 = obb->getExtents().y * obb->getExtents().y;
        float z2 = obb->getExtents().z * obb->getExtents().z;
        const auto extents = obb->getExtents();
        ix = ((y2 + z2) / m_massInverse) * fraction;
        iy = ((x2 + z2) / m_massInverse) * fraction;
        iz = ((x2 + y2) / m_massInverse) * fraction;

        m_inverseInertia = glm::mat3(
                ix, 0.0, 0.0,
                0.0, iy, 0.0,
                0.0, 0.0, iz);

        m_inverseInertia = glm::inverse(m_inverseInertia);
        return;
    }
}

void OGPhysicsComponent::update(double delta) {
    if (!m_owner) {
        throw std::runtime_error("Error: Physics Component has no owner");
    }

    if (!m_owner->getComponent<OGCollisionComponent>()) {
        throw std::runtime_error("Error: Physics Component has no collision component");
    }

    const auto volume = m_owner->getComponent<OGCollisionComponent>()->getCollisionVolume<IVolume>();
    if (!volume) {
        throw std::runtime_error("Error: Physics Component has no volume");
    }

    // Update position based on velocity
    glm::vec3 newPosition = m_owner->getTranslation() + m_velocity * (float) delta;
    m_owner->setTranslation(newPosition);

    // Update rotation based on angular velocity
    glm::quat deltaRotation = glm::quat(0.0f, m_angularVelocity * (float) delta);
    glm::quat newRotation = glm::normalize(deltaRotation * m_owner->getRotation());
    m_owner->setRotation(newRotation);


}

float OGPhysicsComponent::getMass() {
    return m_massInverse;
}

glm::mat3 OGPhysicsComponent::getInertiaTensor() const{
    return m_inverseInertia;
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
