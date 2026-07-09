//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#include "OGCollisionManifold.hpp"

OGCollisionManifold::OGCollisionManifold() {
    m_contacts.clear();
    m_normal = glm::vec3(0.0f);
    m_depth = FLT_MAX;
    m_colliding = false;
    m_swapBodies = false;
    m_bodies[0] = nullptr;
    m_bodies[1] = nullptr;
}

void OGCollisionManifold::reset() {
    m_contacts.clear();
    m_normal = glm::vec3(0.0f);
    m_depth = FLT_MAX;
    m_colliding = false;
    m_swapBodies = false;
    m_bodies[0] = nullptr;
    m_bodies[1] = nullptr;
}