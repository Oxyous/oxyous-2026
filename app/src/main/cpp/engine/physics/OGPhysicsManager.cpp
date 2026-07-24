//
// Created by Mr Steven J Baldwin on 05/07/2026.
//

#include "OGPhysicsManager.hpp"
#include "../collision/CollisionHelper.hpp"
#include "../collision/Collision.hpp"
#include "engine/components/OGCollisionComponent.hpp"
#include "../math/MathHelper.hpp"
#include "engine/GameView.hpp"
#include "engine/Engine.hpp"


OGPhysicsManager::OGPhysicsManager() {

}

OGPhysicsManager::~OGPhysicsManager() {

}

void OGPhysicsManager::step(float deltaTime) {
    // 1. Integrate Forces
    for (auto actor : m_physicsActors) {
        auto phys = actor->getComponent<OGPhysicsComponent>();
        if (phys && phys->isAwake()) {
            phys->integrateForces(deltaTime);
        }
    }

    // 2. Detect Collisions
    m_manifolds.clear();

    // 2.1 Dynamic-Dynamic
    for (auto i = 0; i < m_physicsActors.size(); ++i) {
        auto actorA = m_physicsActors[i];
        auto colA = actorA->getComponent<OGCollisionComponent>();
        auto volA = colA ? colA->getCollisionVolume<IVolume>() : nullptr;
        auto physA = actorA->getComponent<OGPhysicsComponent>();
        if (!physA || !volA) continue;

        for (auto j = i + 1; j < m_physicsActors.size(); ++j) {
            auto actorB = m_physicsActors[j];
            auto colB = actorB->getComponent<OGCollisionComponent>();
            auto volB = colB ? colB->getCollisionVolume<IVolume>() : nullptr;
            auto physB = actorB->getComponent<OGPhysicsComponent>();
            if (!physB || !volB) continue;

            // Broad-phase: Fast AABB intersection check
            if (!volA->getAABB().intersect(volB->getAABB())) continue;

            OGCollisionManifold manifold = volA->resolveCollision(volB);
            if (manifold.m_colliding) {
                manifold.m_bodies[0] = physA;
                manifold.m_bodies[1] = physB;
                if (manifold.m_swapBodies) std::swap(manifold.m_bodies[0], manifold.m_bodies[1]);
                m_manifolds.emplace_back(manifold);
            }
        }

        // 2.2 Dynamic-Static (Environment) with Continuous Collision Detection (Speculative Contacts)
        std::vector<OGPolygon> worldPolygons;

        // Expansion margin for CCD: covers distance moved in one frame
        float speed = glm::length(physA->getVelocity());
        float ccdMargin = speed * deltaTime;

        // Expand AABB for broadphase
        AABBVolume sweptAABB = volA->getAABB();
        sweptAABB.setMin(sweptAABB.getMin() - glm::vec3(ccdMargin));
        sweptAABB.setMax(sweptAABB.getMax() + glm::vec3(ccdMargin));

        // Use octree/BVH to get potential polygons
        ENGINE->getAABBIntersectionByBHV(sweptAABB, worldPolygons); // Assuming queryOctree can take AABB and return polygons
        // Note: if queryOctree only returns entities, we'd need another way,
        // but current implementation seems to have getXXXIntersectionByBHV which works with polygons.

        // Fallback to existing BHV query if octree doesn't support direct polygon retrieval
        if (worldPolygons.empty()) {
            if (auto obb = dynamic_cast<OBBVolume*>(volA)) {
                ENGINE->getObbIntersectionByBHV(*obb, worldPolygons);
            } else if (auto sphere = dynamic_cast<SphereVolume*>(volA)) {
                ENGINE->getSphereIntersectionByBHV(*sphere, worldPolygons);
            } else if (auto capsule = dynamic_cast<CapsuleVolume*>(volA)) {
                ENGINE->getCapsuleIntersectionByBHV(*capsule, worldPolygons);
            }
        }

        for (const auto& poly : worldPolygons) {
            OGCollisionManifold mStatic;
            OGContact contact;
            bool foundContact = false;

            if (auto sphere = dynamic_cast<SphereVolume*>(volA)) {
                if (CollisionHelper::queryPolygonSphereContact(poly, *sphere, ccdMargin, contact)) {
                    foundContact = true;
                }
            } else if (auto capsule = dynamic_cast<CapsuleVolume*>(volA)) {
                if (CollisionHelper::queryPolygonCapsuleContact(poly, *capsule, ccdMargin, contact)) {
                    foundContact = true;
                }
            } else if (auto obb = dynamic_cast<OBBVolume*>(volA)) {
                // OBB speculative contact is more complex, fallback to discrete for now
                mStatic = CollisionHelper::resolveCollision(*obb, poly);
                if (mStatic.m_colliding) {
                    mStatic.m_bodies[0] = physA;
                    mStatic.m_bodies[1] = nullptr;
                    m_manifolds.emplace_back(mStatic);
                }
                continue;
            }

            if (foundContact) {
                mStatic.m_colliding = true;
                mStatic.m_normal = -contact.normal;
                mStatic.m_depth = contact.depth; // positive for overlap, negative for gap
                mStatic.m_contacts.push_back(contact.hitPoint);
                mStatic.m_bodies[0] = physA;
                mStatic.m_bodies[1] = nullptr;
                m_manifolds.emplace_back(mStatic);
            }
        }
    }

    // 3. Resolve Collisions
    if (!m_manifolds.empty()) {
        std::sort(m_manifolds.begin(), m_manifolds.end(),
                  [](const OGCollisionManifold &a, const OGCollisionManifold &b) {
                      return a.m_depth > b.m_depth;
                  });

        // 3.1 Position Correction (Baumgarte) - only for actual penetrations
        for (int iter = 0; iter < 3; iter++) {
            for (const auto &m : m_manifolds) {
                if (m.m_depth > 0) { // Only correct if overlapping
                    updatePositionManifold(m);
                }
            }
        }

        // 3.2 Velocity/Impulse iterations (handles speculative contacts)
        for (int i = 0; i < 10; i++) {
            for (auto &m : m_manifolds) {
                for (size_t k = 0; k < m.m_contacts.size(); k++) {
                    applyRotationImpulse(m, (int) k, deltaTime);
                }
            }
        }
    }

    // 4. Integrate Velocities
    for (auto actor : m_physicsActors) {
        auto phys = actor->getComponent<OGPhysicsComponent>();
        if (phys && phys->isAwake()) {
            phys->integrateVelocity(deltaTime);
        }
    }
}

void OGPhysicsManager::registerPhysicsActor(OGEntity *actorRef) {
    if (actorRef) {
        std::lock_guard<std::mutex> lock(m_physicsMutex);
        m_physicsActors.push_back(actorRef);
    }
}

void OGPhysicsManager::updatePositionManifold(const OGCollisionManifold &manifold) {
    auto bodyA = manifold.m_bodies[0];
    auto bodyB = manifold.m_bodies[1];

    if (!bodyA) return;

    float invMassA = bodyA->getInverseMass();
    float invMassB = bodyB ? bodyB->getInverseMass() : 0.0f;
    float totalInvMass = invMassA + invMassB;

    const float slop = 0.005f;
    const float percent = 0.5f;

    float depth = std::max(manifold.m_depth - slop, 0.0f);
    if (depth <= 0.0f) return;

    glm::vec3 correctionVector = manifold.m_normal * (depth * percent);

    if (totalInvMass > 0.0f) {
        if (invMassA > 0.0f) {
            bodyA->getOwner()->setTranslation(bodyA->getOwner()->getTranslation() - correctionVector * (invMassA / totalInvMass));
        }
        if (bodyB && invMassB > 0.0f) {
            bodyB->getOwner()->setTranslation(bodyB->getOwner()->getTranslation() + correctionVector * (invMassB / totalInvMass));
        }
    } else {
        if (!bodyB) {
            bodyA->getOwner()->setTranslation(bodyA->getOwner()->getTranslation() - correctionVector);
        } else {
            bodyA->getOwner()->setTranslation(bodyA->getOwner()->getTranslation() - correctionVector * 0.5f);
            bodyB->getOwner()->setTranslation(bodyB->getOwner()->getTranslation() + correctionVector * 0.5f);
        }
    }
}

void OGPhysicsManager::applyRotationImpulse(const OGCollisionManifold &manifold, int c, float dt) {
    auto bodyA = manifold.m_bodies[0];
    auto bodyB = manifold.m_bodies[1];

    if (!bodyA) return;

    auto invMassA = bodyA->getInverseMass();
    auto invMassB = bodyB ? bodyB->getInverseMass() : 0.0f;
    auto totalInvMass = invMassA + invMassB;

    if (totalInvMass == 0.0f) return;

    glm::vec3 r1 = manifold.m_contacts[c] - bodyA->getOwner()->getTranslation();
    glm::vec3 r2 = bodyB ? (manifold.m_contacts[c] - bodyB->getOwner()->getTranslation()) : glm::vec3(0.0f);

    glm::mat3 i1 = bodyA->getInverseInertiaWorld();
    glm::mat3 i2 = bodyB ? bodyB->getInverseInertiaWorld() : glm::mat3(0.0f);

    glm::vec3 velB = bodyB ? bodyB->getVelocity() : glm::vec3(0.0f);
    glm::vec3 angB = bodyB ? bodyB->getAngularVelocity() : glm::vec3(0.0f);

    glm::vec3 relativeVelocity =
            (velB + glm::cross(angB, r2)) -
            (bodyA->getVelocity() + glm::cross(bodyA->getAngularVelocity(), r1));

    glm::vec3 relativeNormal = glm::normalize(manifold.m_normal);

    float velAlongNormal = glm::dot(relativeVelocity, relativeNormal);

    // CCD Speculative Contact logic:
    // If separated (depth < 0), we only apply impulse if velocity will cause collision
    float bias = 0.0f;
    if (manifold.m_depth < 0) {
        bias = manifold.m_depth / dt;
    }

    // Don't apply impulse if moving away fast enough
    if (velAlongNormal + bias > 0.0f) return;

    float e = bodyB ? fminf(bodyA->getRestitution(), bodyB->getRestitution()) : bodyA->getRestitution();
    // Disable restitution for speculative contacts to prevent "jumping"
    if (manifold.m_depth < 0) e = 0.0f;
    else if (std::abs(velAlongNormal) < 0.2f) e = 0.0f;

    float numerator = (-(1.0f + e) * velAlongNormal - bias);

    glm::vec3 d2 = glm::cross(i1 * glm::cross(r1, relativeNormal), r1);
    glm::vec3 d3 = bodyB ? glm::cross(i2 * glm::cross(r2, relativeNormal), r2) : glm::vec3(0.0f);
    float denominator = totalInvMass + glm::dot(relativeNormal, d2 + d3);

    float j = (denominator != 0.0f) ? numerator / denominator : 0.0f;
    if (!manifold.m_contacts.empty()) j /= (float) manifold.m_contacts.size();

    glm::vec3 impulse = relativeNormal * j;

    bodyA->setVelocity(bodyA->getVelocity() - impulse * invMassA);
    bodyA->setAngularVelocity(bodyA->getAngularVelocity() - (i1 * glm::cross(r1, impulse)));

    if (bodyB) {
        bodyB->setVelocity(bodyB->getVelocity() + impulse * invMassB);
        bodyB->setAngularVelocity(bodyB->getAngularVelocity() + (i2 * glm::cross(r2, impulse)));
    }

    // Friction (skip for speculative gap contacts for simplicity)
    if (manifold.m_depth >= 0) {
        relativeVelocity = (bodyB ? (bodyB->getVelocity() + glm::cross(bodyB->getAngularVelocity(), r2)) : glm::vec3(0.0f)) -
                           (bodyA->getVelocity() + glm::cross(bodyA->getAngularVelocity(), r1));
        glm::vec3 t = relativeVelocity - (relativeNormal * glm::dot(relativeVelocity, relativeNormal));

        if (glm::dot(t, t) > 1e-6f) {
            t = glm::normalize(t);
            numerator = -glm::dot(relativeVelocity, t);
            d2 = glm::cross(i1 * glm::cross(r1, t), r1);
            d3 = bodyB ? glm::cross(i2 * glm::cross(r2, t), r2) : glm::vec3(0.0f);
            denominator = totalInvMass + glm::dot(t, d2 + d3);

            float jt = (denominator != 0.0f) ? numerator / denominator : 0.0f;
            if (!manifold.m_contacts.empty()) jt /= (float) manifold.m_contacts.size();

            float friction = bodyB ? sqrt(bodyA->getFriction() * bodyB->getFriction()) : bodyA->getFriction();
            jt = glm::clamp(jt, -j * friction, j * friction);

            glm::vec3 tangentImpulse = t * jt;
            bodyA->setVelocity(bodyA->getVelocity() - tangentImpulse * invMassA);
            bodyA->setAngularVelocity(bodyA->getAngularVelocity() - (i1 * glm::cross(r1, tangentImpulse)));

            if (bodyB) {
                bodyB->setVelocity(bodyB->getVelocity() + tangentImpulse * invMassB);
                bodyB->setAngularVelocity(bodyB->getAngularVelocity() + (i2 * glm::cross(r2, tangentImpulse)));
            }
        }
    }

    bodyA->setAwake(true);
    if (bodyB) bodyB->setAwake(true);
}

void OGPhysicsManager::start() {
    m_executing = true;
    m_thread = std::thread(&OGPhysicsManager::run, this);
}

void OGPhysicsManager::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;
    const float fixedDeltaTime = 0.0078125f; // 128Hz

    while (m_executing) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        lastTime = currentTime;

        float frameTime = elapsed.count();
        if (frameTime > 0.25f) frameTime = 0.25f;

        accumulator += frameTime;

        while (accumulator >= fixedDeltaTime) {
            {
                std::lock_guard<std::mutex> lock(m_physicsMutex);
                step(fixedDeltaTime);
            }
            accumulator -= fixedDeltaTime;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}
