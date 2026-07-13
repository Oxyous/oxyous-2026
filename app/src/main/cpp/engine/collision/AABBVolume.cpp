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

    float distance = glm::dot(plane.m_normal, center) - plane.m_distance;

    return std::abs(distance) <= projectionRadius;
}

OGCollisionManifold AABBVolume::resolveCollision(IVolume *volume) {
    return OGCollisionManifold();
}

void AABBVolume::transform(const glm::mat4 &transform) {

}

