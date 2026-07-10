//
// Created by Mr Steven J Baldwin on 05/07/2026.
//

#include "OGPhysicsManager.hpp"
#include "../collision/CollisionHelper.hpp"
#include "../collision/Collision.hpp"
#include "engine/components/OGCollisionComponent.hpp"
#include "../math/MathHelper.hpp"


OGPhysicsManager::OGPhysicsManager() {

}

OGPhysicsManager::~OGPhysicsManager() {

}

void OGPhysicsManager::update(float deltaTime) {
    if (!m_executing) return;

    // 1. Integrate Forces
    for (auto actor : m_physicsActors) {
        auto phys = actor->getComponent<OGPhysicsComponent>();
        if (phys) phys->integrateForces(deltaTime);
    }

    // 2. Detect Collisions
    OGCollisionManifold manifold;
    m_manifolds.clear();

    for (auto i = 0; i < m_physicsActors.size(); ++i) {
        auto colA = m_physicsActors[i]->getComponent<OGCollisionComponent>();
        auto volA = colA->getCollisionVolume<IVolume>();

        for (auto j = i + 1; j < m_physicsActors.size(); ++j) {
            auto colB = m_physicsActors[j]->getComponent<OGCollisionComponent>();
            auto volB = colB->getCollisionVolume<IVolume>();

            auto physA = colA->getOwner()->getComponent<OGPhysicsComponent>();
            auto physB = colB->getOwner()->getComponent<OGPhysicsComponent>();

            if (physA->getMass() == 0.0f && physB->getMass() == 0.0f) {
                continue;
            }

            manifold = volA->resolveCollision(volB);

            if (manifold.m_colliding) {
                manifold.m_bodies[0] = physA;
                manifold.m_bodies[1] = physB;

                if (manifold.m_swapBodies) {
                    auto tmp = manifold.m_bodies[0];
                    manifold.m_bodies[0] = manifold.m_bodies[1];
                    manifold.m_bodies[1] = tmp;
                }

                m_manifolds.emplace_back(manifold);
            }
        }
    }

    // 3. Resolve Collisions (Position then Impulses)
    std::sort(m_manifolds.begin(), m_manifolds.end(),
              [](const OGCollisionManifold &a, const OGCollisionManifold &b) {
                  return a.m_depth > b.m_depth;
              });

    for (const auto &m: m_manifolds) {
        updatePositionManifold(m);
    }

    for (int i = 0; i < 8; i++) {
        for (auto &m: m_manifolds) {
            for (size_t k = 0; k < m.m_contacts.size(); k++) {
                applyRotationImpulse(m, (int) k);
            }
        }
    }

    // 4. Integrate Velocities (Move objects)
    for (auto actor : m_physicsActors) {
        auto phys = actor->getComponent<OGPhysicsComponent>();
        if (phys) phys->integrateVelocity(deltaTime);
    }
}

void OGPhysicsManager::registerPhysicsActor(OGEntity *actorRef) {
    if (actorRef) {
        m_physicsActors.push_back(actorRef);
    }
}

void OGPhysicsManager::updatePositionManifold(const OGCollisionManifold &manifold) {
    auto bodyA = manifold.m_bodies[0];
    auto bodyB = manifold.m_bodies[1];

    if (!bodyA || !bodyB) {
        return;
    }

    float totalMass = bodyA->getMass() + bodyB->getMass();

    if (totalMass <= 0.0f) {
        return;
    }

    float massRatioA = bodyA->getMass() / totalMass;
    float massRatioB = bodyB->getMass() / totalMass;

    float percent = 0.5f; // Baumgarte stabilization
    glm::vec3 correctionVector = manifold.m_normal * (manifold.m_depth * percent);

    if (bodyA->getMass() > 0.0f) {
        glm::vec3 newPositionA =
                bodyA->getOwner()->getTranslation() - correctionVector * massRatioA;
        bodyA->getOwner()->setTranslation(newPositionA);
    }

    if (bodyB->getMass() > 0.0f) {
        glm::vec3 newPositionB =
                bodyB->getOwner()->getTranslation() + correctionVector * massRatioB;
        bodyB->getOwner()->setTranslation(newPositionB);
    }
}

void OGPhysicsManager::applyRotationImpulse(const OGCollisionManifold &manifold, int c) {
    auto bodyA = manifold.m_bodies[0];
    auto bodyB = manifold.m_bodies[1];

    auto massA = manifold.m_bodies[0]->getMass();
    auto massB = manifold.m_bodies[1]->getMass();
    auto massSum = massA + massB;

    if (massSum == 0.0f) {
        return;
    }

    glm::vec3 r1 = manifold.m_contacts[c] - bodyA->getOwner()->getTranslation();
    glm::vec3 r2 = manifold.m_contacts[c] - bodyB->getOwner()->getTranslation();

    glm::mat3 i1 = manifold.m_bodies[0]->getInertiaTensor();
    glm::mat3 i2 = manifold.m_bodies[1]->getInertiaTensor();

    glm::vec3 relativeVelocity =
            (bodyB->getVelocity() + glm::cross(bodyB->getAngularVelocity(), r2)) -
            (bodyA->getVelocity() + glm::cross(bodyA->getAngularVelocity(), r1));

    glm::vec3 relativeNormal = manifold.m_normal;
    relativeNormal = glm::normalize(relativeNormal);

    if (glm::dot(relativeVelocity, relativeNormal) > 0.0f) {
        return;
    }

    float e = fminf(bodyA->getRestitution(), bodyB->getRestitution());
    if (std::abs(glm::dot(relativeVelocity, relativeNormal)) < 0.2f) {
        e = 0.0f;
    }

    float numerator = (-(1.0f + e) * glm::dot(relativeVelocity, relativeNormal));

    float d1 = massSum;
    glm::vec3 d2 = glm::cross(i1 * glm::cross(r1, relativeNormal), r1);
    glm::vec3 d3 = glm::cross(i2 * glm::cross(r2, relativeNormal), r2);
    float denominator = d1 + glm::dot(relativeNormal, d2 + d3);

    float j = (denominator != 0.0f) ? numerator / denominator : 0.0f;

    glm::vec3 impulse = relativeNormal * j;

    auto velA = bodyA->getVelocity();
    auto velB = bodyB->getVelocity();

    auto angA = bodyA->getAngularVelocity();
    auto angB = bodyB->getAngularVelocity();

    bodyA->setVelocity(velA - impulse * massA);
    bodyB->setVelocity(velB + impulse * massB);

    bodyA->setAngularVelocity(angA - (i1 * glm::cross(r1, impulse)));
    bodyB->setAngularVelocity(angB + (i2 * glm::cross(r2, impulse)));

    glm::vec3 t = relativeVelocity - (relativeNormal * glm::dot(relativeVelocity, relativeNormal));

    if (Math::isClose(glm::dot(t, t), 0.0f))
        return;

    t = glm::normalize(t);

    numerator = -glm::dot(relativeVelocity, t);
    d1 = massSum;
    d2 = glm::cross(i1 * glm::cross(r1, t), r1);
    d3 = glm::cross(i2 * glm::cross(r2, t), r2);

    denominator = d1 + glm::dot(t, d2 + d3);

    float jt = (denominator != 0.0f) ? numerator / denominator : 0.0f;

    if (Math::isClose(jt, 0.0)) return;

    float friction = sqrt(bodyA->getFriction() * bodyB->getFriction());
    if (jt > j * friction) {
        jt = j * friction;
    } else if (jt < -j * friction) {
        jt = -j * friction;
    }

    glm::vec3 tangentImpulse = t * jt;

    velA = bodyA->getVelocity();
    velB = bodyB->getVelocity();

    bodyA->setVelocity(velA - tangentImpulse * massA);
    bodyB->setVelocity(velB + tangentImpulse * massB);

    angA = bodyA->getAngularVelocity();
    angB = bodyB->getAngularVelocity();

    bodyA->setAngularVelocity(angA - (i1 * glm::cross(r1, tangentImpulse)));
    bodyB->setAngularVelocity(angB + (i2 * glm::cross(r2, tangentImpulse)));

    bodyA->setAwake(true);
    bodyB->setAwake(true);
}

void OGPhysicsManager::start() {
    m_executing = true;
}
