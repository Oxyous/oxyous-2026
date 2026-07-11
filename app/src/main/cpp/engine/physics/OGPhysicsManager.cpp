//
// Created by Mr Steven J Baldwin on 05/07/2026.
//

#include "OGPhysicsManager.hpp"
#include "../collision/CollisionHelper.hpp"
#include "../collision/Collision.hpp"
#include "engine/components/OGCollisionComponent.hpp"
#include "../math/MathHelper.hpp"
#include "engine/GameView.hpp"


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
    m_manifolds.clear();

    // 2.1 Dynamic-Dynamic
    for (auto i = 0; i < m_physicsActors.size(); ++i) {
        auto actorA = m_physicsActors[i];
        auto colA = actorA->getComponent<OGCollisionComponent>();
        auto volA = colA->getCollisionVolume<IVolume>();
        auto physA = actorA->getComponent<OGPhysicsComponent>();

        for (auto j = i + 1; j < m_physicsActors.size(); ++j) {
            auto actorB = m_physicsActors[j];
            auto colB = actorB->getComponent<OGCollisionComponent>();
            auto volB = colB->getCollisionVolume<IVolume>();
            auto physB = actorB->getComponent<OGPhysicsComponent>();

            if (physA->getMass() == 0.0f && physB->getMass() == 0.0f) continue;

            OGCollisionManifold manifold = volA->resolveCollision(volB);
            if (manifold.m_colliding) {
                manifold.m_bodies[0] = physA;
                manifold.m_bodies[1] = physB;
                if (manifold.m_swapBodies) std::swap(manifold.m_bodies[0], manifold.m_bodies[1]);
                m_manifolds.emplace_back(manifold);
            }
        }

        // 2.2 Dynamic-Static (Environment)
        if (physA->getMass() > 0.0f) {
            std::vector<OGPolygon> worldPolygons;
            if (auto obb = dynamic_cast<OBBVolume*>(volA)) {
                GAME_VIEW->getObbIntersectionByBHV(*obb, worldPolygons);
            } else if (auto sphere = dynamic_cast<SphereVolume*>(volA)) {
                GAME_VIEW->getSphereIntersectionByBHV(*sphere, worldPolygons);
            } else if (auto capsule = dynamic_cast<CapsuleVolume*>(volA)) {
                GAME_VIEW->getCapsuleIntersectionByBHV(*capsule, worldPolygons);
            }

            for (const auto& poly : worldPolygons) {
                OGCollisionManifold mStatic;
                if (auto obb = dynamic_cast<OBBVolume*>(volA)) {
                    mStatic = CollisionHelper::resolveCollision(*obb, poly);
                } else if (auto sphere = dynamic_cast<SphereVolume*>(volA)) {
                    OGContact contact;
                    if (CollisionHelper::resolvePolygonSphereCollision(poly, *sphere, contact)) {
                        mStatic.m_colliding = true;
                        mStatic.m_normal = -contact.normal; // Point from Sphere to Polygon
                        mStatic.m_depth = contact.depth;
                        mStatic.m_contacts.push_back(contact.hitPoint);
                    }
                } else if (auto capsule = dynamic_cast<CapsuleVolume*>(volA)) {
                    OGContact contact;
                    if (CollisionHelper::resolvePolygonCapsuleCollision(poly, *capsule, contact)) {
                        mStatic.m_colliding = true;
                        mStatic.m_normal = -contact.normal; // Point from Capsule to Polygon
                        mStatic.m_depth = contact.depth;
                        mStatic.m_contacts.push_back(contact.hitPoint);
                    }
                }

                if (mStatic.m_colliding) {
                    mStatic.m_bodies[0] = physA;
                    mStatic.m_bodies[1] = nullptr;
                    m_manifolds.emplace_back(mStatic);
                }
            }
        }
    }

    // 3. Resolve Collisions (Sequential Impulses)
    std::sort(m_manifolds.begin(), m_manifolds.end(),
              [](const OGCollisionManifold &a, const OGCollisionManifold &b) {
                  return a.m_depth > b.m_depth;
              });

    // 3.1 Position Correction (Baumgarte)
    for (const auto &m : m_manifolds) {
        updatePositionManifold(m);
    }

    // 3.2 Velocity/Impulse iterations
    for (int i = 0; i < 10; i++) {
        for (auto &m : m_manifolds) {
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

    if (!bodyA) return;

    float massA = bodyA->getMass();
    float massB = bodyB ? bodyB->getMass() : 0.0f;
    float totalMass = massA + massB;

    if (totalMass <= 0.0f) return;

    float massRatioA = massA / totalMass;
    float massRatioB = massB / totalMass;

    float percent = 0.5f; // Baumgarte stabilization
    glm::vec3 correctionVector = manifold.m_normal * (manifold.m_depth * percent);

    if (massA > 0.0f) {
        bodyA->getOwner()->setTranslation(bodyA->getOwner()->getTranslation() - correctionVector * massRatioA);
    }

    if (bodyB && massB > 0.0f) {
        bodyB->getOwner()->setTranslation(bodyB->getOwner()->getTranslation() + correctionVector * massRatioB);
    }
}

void OGPhysicsManager::applyRotationImpulse(const OGCollisionManifold &manifold, int c) {
    auto bodyA = manifold.m_bodies[0];
    auto bodyB = manifold.m_bodies[1];

    if (!bodyA) return;

    auto massA = bodyA->getMass();
    auto massB = bodyB ? bodyB->getMass() : 0.0f;
    auto massSum = massA + massB;

    if (massSum == 0.0f) {
        return;
    }

    glm::vec3 r1 = manifold.m_contacts[c] - bodyA->getOwner()->getTranslation();
    glm::vec3 r2 = bodyB ? (manifold.m_contacts[c] - bodyB->getOwner()->getTranslation()) : glm::vec3(0.0f);

    glm::mat3 i1 = bodyA->getInertiaTensor();
    glm::mat3 i2 = bodyB ? bodyB->getInertiaTensor() : glm::mat3(0.0f);

    glm::vec3 velB = bodyB ? bodyB->getVelocity() : glm::vec3(0.0f);
    glm::vec3 angB = bodyB ? bodyB->getAngularVelocity() : glm::vec3(0.0f);

    glm::vec3 relativeVelocity =
            (velB + glm::cross(angB, r2)) -
            (bodyA->getVelocity() + glm::cross(bodyA->getAngularVelocity(), r1));

    glm::vec3 relativeNormal = manifold.m_normal;
    relativeNormal = glm::normalize(relativeNormal);

    if (glm::dot(relativeVelocity, relativeNormal) > 0.0f) {
        return;
    }

    float e = bodyB ? fminf(bodyA->getRestitution(), bodyB->getRestitution()) : bodyA->getRestitution();
    if (std::abs(glm::dot(relativeVelocity, relativeNormal)) < 0.2f) {
        e = 0.0f;
    }

    float numerator = (-(1.0f + e) * glm::dot(relativeVelocity, relativeNormal));

    float d1 = massSum;
    glm::vec3 d2 = glm::cross(i1 * glm::cross(r1, relativeNormal), r1);
    glm::vec3 d3 = bodyB ? glm::cross(i2 * glm::cross(r2, relativeNormal), r2) : glm::vec3(0.0f);
    float denominator = d1 + glm::dot(relativeNormal, d2 + d3);

    float j = (denominator != 0.0f) ? numerator / denominator : 0.0f;
    if (manifold.m_contacts.size() > 0.0f && j != 0.0f) {
        j /= (float) manifold.m_contacts.size();
    }
    glm::vec3 impulse = relativeNormal * j;

    bodyA->setVelocity(bodyA->getVelocity() - impulse * massA);
    bodyA->setAngularVelocity(bodyA->getAngularVelocity() - (i1 * glm::cross(r1, impulse)));

    if (bodyB) {
        bodyB->setVelocity(bodyB->getVelocity() + impulse * massB);
        bodyB->setAngularVelocity(bodyB->getAngularVelocity() + (i2 * glm::cross(r2, impulse)));
    }

    glm::vec3 t = relativeVelocity - (relativeNormal * glm::dot(relativeVelocity, relativeNormal));

    if (Math::isClose(glm::dot(t, t), 0.0f))
        return;

    t = glm::normalize(t);

    numerator = -glm::dot(relativeVelocity, t);
    d1 = massSum;
    d2 = glm::cross(i1 * glm::cross(r1, t), r1);
    d3 = bodyB ? glm::cross(i2 * glm::cross(r2, t), r2) : glm::vec3(0.0f);

    denominator = d1 + glm::dot(t, d2 + d3);

    float jt = (denominator != 0.0f) ? numerator / denominator : 0.0f;
    if (Math::isClose(jt, 0.0)) return;

    float friction = bodyB ? sqrt(bodyA->getFriction() * bodyB->getFriction()) : bodyA->getFriction();
    if (jt > j * friction) {
        jt = j * friction;
    } else if (jt < -j * friction) {
        jt = -j * friction;
    }

    glm::vec3 tangentImpulse = t * jt;

    bodyA->setVelocity(bodyA->getVelocity() - tangentImpulse * massA);
    bodyA->setAngularVelocity(bodyA->getAngularVelocity() - (i1 * glm::cross(r1, tangentImpulse)));

    if (bodyB) {
        bodyB->setVelocity(bodyB->getVelocity() + tangentImpulse * massB);
        bodyB->setAngularVelocity(bodyB->getAngularVelocity() + (i2 * glm::cross(r2, tangentImpulse)));
    }

    bodyA->setAwake(true);
    if (bodyB) bodyB->setAwake(true);
}

void OGPhysicsManager::start() {
    m_executing = true;
    m_thread = std::thread(&OGPhysicsManager::run, this);
}

void OGPhysicsManager::integrate(float delta) {

    // 1. Integrate Forces
    for (auto actor: m_physicsActors) {
        auto phys = actor->getComponent<OGPhysicsComponent>();
        if (phys) phys->integrateForces(delta);
    }

    // 2. Detect Collisions
    m_manifolds.clear();

    for (auto i = 0; i < m_physicsActors.size(); ++i) {
        auto actor = m_physicsActors[i];
        auto col = actor->getComponent<OGCollisionComponent>();
        auto vol = col->getCollisionVolume<IVolume>();
        auto phys = actor->getComponent<OGPhysicsComponent>();

        if (phys->getMass() == 0.0f) continue;

        std::vector<OGPolygon> worldPolygons;
        if (auto obb = dynamic_cast<OBBVolume*>(vol)) {
            GAME_VIEW->getObbIntersectionByBHV(*obb, worldPolygons);
        } else if (auto sphere = dynamic_cast<SphereVolume*>(vol)) {
            GAME_VIEW->getSphereIntersectionByBHV(*sphere, worldPolygons);
        } else if (auto capsule = dynamic_cast<CapsuleVolume*>(vol)) {
            GAME_VIEW->getCapsuleIntersectionByBHV(*capsule, worldPolygons);
        }

        for (const auto &poly: worldPolygons) {
            OGCollisionManifold mStatic;
            if (auto obb = dynamic_cast<OBBVolume*>(vol)) {
                mStatic = CollisionHelper::resolveCollision(*obb, poly);
            } else if (auto sphere = dynamic_cast<SphereVolume*>(vol)) {
                OGContact contact;
                if (CollisionHelper::resolvePolygonSphereCollision(poly, *sphere, contact)) {
                    mStatic.m_colliding = true;
                    mStatic.m_normal = -contact.normal;
                    mStatic.m_depth = contact.depth;
                    mStatic.m_contacts.push_back(contact.hitPoint);
                }
            } else if (auto capsule = dynamic_cast<CapsuleVolume*>(vol)) {
                OGContact contact;
                if (CollisionHelper::resolvePolygonCapsuleCollision(poly, *capsule, contact)) {
                    mStatic.m_colliding = true;
                    mStatic.m_normal = -contact.normal;
                    mStatic.m_depth = contact.depth;
                    mStatic.m_contacts.push_back(contact.hitPoint);
                }
            }

            if (mStatic.m_colliding) {
                mStatic.m_bodies[0] = phys;
                mStatic.m_bodies[1] = nullptr;
                m_manifolds.emplace_back(mStatic);
            }
        }
    }

    // 3. Resolve Collisions (Position then Impulses)
    std::sort(m_manifolds.begin(), m_manifolds.end(),
              [](const OGCollisionManifold &a, const OGCollisionManifold &b) {
                  return a.m_depth > b.m_depth;
              });

    for (const auto& m : m_manifolds) {
        if (m.m_bodies[1] == nullptr) {
            updateStaticPositionManifold(m);
        }
    }

    for (int i = 0; i < 8; i++) {
        for (auto &m: m_manifolds) {
            if (m.m_bodies[1] == nullptr) {
                for (size_t k = 0; k < m.m_contacts.size(); k++) {
                    applyStaticRotationImpulse(m, (int) k);
                }
            }
        }
    }

    // 4. Integrate Velocities (Move objects)
    for (auto actor: m_physicsActors) {
        auto phys = actor->getComponent<OGPhysicsComponent>();
        if (phys) phys->integrateVelocity(delta);
    }
}

void OGPhysicsManager::updateStaticPositionManifold(const OGCollisionManifold &manifold) {
    auto bodyA = manifold.m_bodies[0];

    if (!bodyA) {
        return;
    }

    float totalMass = bodyA->getMass();

    if (totalMass <= 0.0f) {
        return;
    }

    float massRatioA = bodyA->getMass() / totalMass;

    float percent = 0.5f; // Baumgarte stabilization
    glm::vec3 correctionVector = manifold.m_normal * (manifold.m_depth * percent);

    if (bodyA->getMass() > 0.0f) {
        glm::vec3 newPositionA =
                bodyA->getOwner()->getTranslation() - correctionVector * massRatioA;
        bodyA->getOwner()->setTranslation(newPositionA);
    }
}

void OGPhysicsManager::applyStaticRotationImpulse(const OGCollisionManifold &manifold, int c) {
    auto bodyA = manifold.m_bodies[0];

    auto massA = manifold.m_bodies[0]->getMass();

    if (massA == 0.0f) {
        return;
    }

    glm::vec3 r1 = manifold.m_contacts[c] - bodyA->getOwner()->getTranslation();

    glm::mat3 i1 = bodyA->getInertiaTensor();

    glm::vec3 relativeVelocity =
            -(bodyA->getVelocity() + glm::cross(bodyA->getAngularVelocity(), r1));

    glm::vec3 relativeNormal = manifold.m_normal;
    relativeNormal = glm::normalize(relativeNormal);

    if (glm::dot(relativeVelocity, relativeNormal) > 0.0f) {
        return;
    }

    float e = bodyA->getRestitution();
    if (std::abs(glm::dot(relativeVelocity, relativeNormal)) < 0.2f) {
        e = 0.0f;
    }

    float numerator = (-(1.0f + e) * glm::dot(relativeVelocity, relativeNormal));

    float d1 = massA;
    glm::vec3 d2 = glm::cross(i1 * glm::cross(r1, relativeNormal), r1);
    float denominator = d1 + glm::dot(relativeNormal, d2);

    float j = (denominator != 0.0f) ? numerator / denominator : 0.0f;

    glm::vec3 impulse = relativeNormal * j;

    auto velA = bodyA->getVelocity();
    auto angA = bodyA->getAngularVelocity();

    bodyA->setVelocity(velA - impulse * massA);
    bodyA->setAngularVelocity(angA - (i1 * glm::cross(r1, impulse)));

    glm::vec3 relativeVelocityAfterImpulse =
            -(bodyA->getVelocity() + glm::cross(bodyA->getAngularVelocity(), r1));

    glm::vec3 t = relativeVelocityAfterImpulse - (relativeNormal * glm::dot(relativeVelocityAfterImpulse, relativeNormal));

    if (Math::isClose(glm::dot(t, t), 0.0f))
        return;

    t = glm::normalize(t);

    numerator = -glm::dot(relativeVelocityAfterImpulse, t);
    d1 = massA;
    d2 = glm::cross(i1 * glm::cross(r1, t), r1);

    denominator = d1 + glm::dot(t, d2);

    float jt = (denominator != 0.0f) ? numerator / denominator : 0.0f;
    if (Math::isClose(jt, 0.0)) return;

    float friction = bodyA->getFriction(); // Static geometry friction assumed 1.0 or handled by body
    if (jt > j * friction) {
        jt = j * friction;
    } else if (jt < -j * friction) {
        jt = -j * friction;
    }

    glm::vec3 tangentImpulse = t * jt;

    velA = bodyA->getVelocity();
    angA = bodyA->getAngularVelocity();

    bodyA->setVelocity(velA - tangentImpulse * massA);
    bodyA->setAngularVelocity(angA - (i1 * glm::cross(r1, tangentImpulse)));

    bodyA->setAwake(true);
}

void OGPhysicsManager::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;
    const float fixedDeltaTime = 0.008f; // 125Hz sub-stepping for better stability

    while (m_executing) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        lastTime = currentTime;

        float frameTime = elapsed.count();
        if (frameTime > 0.25f) frameTime = 0.25f; // Cap to avoid "spiral of death"

        accumulator += frameTime;

        while (accumulator >= fixedDeltaTime) {
            integrate(fixedDeltaTime);
            update(fixedDeltaTime);
            accumulator -= fixedDeltaTime;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
