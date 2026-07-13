//
// Created by Mr Steven J Baldwin on 28/06/2026.
//

#include "Collision.hpp"
#include "../physics/OGCollisionManifold.hpp"


bool AABBVolume::intersect(const Ray& ray, RaycastHit& hit) const
{
    return true;
}

bool AABBVolume::intersect(const SphereVolume& sphereVolume) const {
    return true;
}


/* Intersect AABB with Volume */
bool AABBVolume::intersect(const AABBVolume& aabb) const
{
    return (m_min.x <= aabb.m_max.x && m_max.x >= aabb.m_min.x) &&
           (m_min.y <= aabb.m_max.y && m_max.y >= aabb.m_min.y) &&
           (m_min.z <= aabb.m_max.z && m_max.z >= aabb.m_min.z);
}

/* Intersect OBB with Volume */
bool AABBVolume::intersect(const OBBVolume& obb) const {
    return true;
}

bool AABBVolume::intersect(const PlaneVolume &plane) const {
    glm::vec3 center = (m_min + m_max) * 0.5f;
    glm::vec3 extents = m_max - center;

    float projectionRadius = extents.x * std::abs(plane.m_normal.x) +
                             extents.y * std::abs(plane.m_normal.y) +
                             extents.z * std::abs(plane.m_normal.z);

    float distance = glm::dot(plane.m_normal, center) + plane.m_distance;

    return std::abs(distance) <= projectionRadius;
}

OGCollisionManifold AABBVolume::resolveCollision(IVolume *volume) {
    return OGCollisionManifold();
}

void AABBVolume::transform(const glm::mat4 &transform) {
    glm::vec3 corners[8] = {
        {m_min.x, m_min.y, m_min.z}, // 000
        {m_max.x, m_min.y, m_min.z}, // 100
        {m_min.x, m_max.y, m_min.z}, // 010
        {m_max.x, m_max.y, m_min.z}, // 110
        {m_min.x, m_min.y, m_max.z}, // 001
        {m_max.x, m_min.y, m_max.z}, // 101
        {m_min.x, m_max.y, m_max.z}, // 011
        {m_max.x, m_max.y, m_max.z}  // 111
    };

    glm::vec3 newMin(FLT_MAX);
    glm::vec3 newMax(-FLT_MAX);

    for (int i = 0; i < 8; i++) {
        glm::vec3 transformed = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
        newMin = glm::min(newMin, transformed);
        newMax = glm::max(newMax, transformed);
    }

    m_min = newMin;
    m_max = newMax;
}

