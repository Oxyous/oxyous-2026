//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#ifndef OXYOUS_2026_OGCOLLISIONMANIFOLD_HPP
#define OXYOUS_2026_OGCOLLISIONMANIFOLD_HPP

#include "../../includes.hpp"
#include "engine/components/OGPhysicsComponent.hpp"

class OGCollisionManifold {
public:
    OGCollisionManifold();
    ~OGCollisionManifold() = default;
public:
    void reset();
public:
    bool m_colliding;
    float m_depth;
    glm::vec3 m_normal;
    bool m_swapBodies;
    OGPhysicsComponent* m_bodies[2]{};
    std::vector<glm::vec3> m_contacts;
};


#endif //OXYOUS_2026_OGCOLLISIONMANIFOLD_HPP
