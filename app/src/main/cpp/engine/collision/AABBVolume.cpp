//
// Created by Mr Steven J Baldwin on 28/06/2026.
//

#include "Collision.hpp"

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
    return true;
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

    float distance = glm::dot(plane.m_normal, center) - plane.m_distance;

    return std::abs(distance) <= projectionRadius;
}

