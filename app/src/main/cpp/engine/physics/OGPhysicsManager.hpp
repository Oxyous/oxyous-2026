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

    /** Update Physics */
    void update(float deltaTime);

    /** Add Actor Reference to physics system */
    void registerPhysicsActor(OGEntity* actorRef);

    /** Start the physics simulation */
    void start();

    /** Integrate physics state */
    void integrate(float delta);

private:
    void updatePositionManifold(const OGCollisionManifold& manifold);

    void applyRotationImpulse(const OGCollisionManifold& manifold, int c);

    void updateStaticPositionManifold(const OGCollisionManifold& manifold);

    void applyStaticRotationImpulse(const OGCollisionManifold& manifold, int c);

protected:
    float m_gravity = -9.81f;
    std::vector<OGEntity*> m_physicsActors;
    std::vector<OGCollisionManifold> m_manifolds;
private:
    std::thread m_thread;
    std::atomic<bool> m_executing = false;

    void run();
};

#define PHYSICS OGSingleton<OGPhysicsManager>::getInstance()

#endif //OXYOUS_2026_OGPHYSICSMANAGER_HPP
