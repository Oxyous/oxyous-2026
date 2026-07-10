//
// Created by Mr Steven J Baldwin on 05/07/2026.
//

#ifndef OXYOUS_2026_OGPHYSICSMANAGER_HPP
#define OXYOUS_2026_OGPHYSICSMANAGER_HPP

#include <vector>
#include <memory>
#include "OGCollisionManifold.hpp"
#include "../../system/OGSingleton.hpp"

class OBBVolume;
class SphereVolume;

class OGPhysicsManager {
public:
    OGPhysicsManager();
    ~OGPhysicsManager();

    void update(float deltaTime);

    OGCollisionManifold resolveCollision(const OBBVolume& obbA, const OBBVolume& obbB);
    OGCollisionManifold resolveCollision(const OBBVolume& obb, const SphereVolume& sphere);
    OGCollisionManifold resolveCollision(const SphereVolume& sphereA, const SphereVolume& sphereB);

    /** Add Actor Reference to physics system */
    void registerPhysicsActor(OGEntity* actorRef);

    /** */
    void start();

private:
    void resolveCollisions();
    void integrate(float deltaTime);

    void updatePositionManifold(const OGCollisionManifold& manifold);

    void applyRotationImpulse(const OGCollisionManifold& manifold, int c);

protected:
    float m_gravity = -9.81f;
    std::vector<OGEntity*> m_physicsActors;
    std::vector<OGCollisionManifold> m_manifolds;
    bool m_executing = false;
};

#define PHYSICS OGSingleton<OGPhysicsManager>::getInstance()

#endif //OXYOUS_2026_OGPHYSICSMANAGER_HPP
