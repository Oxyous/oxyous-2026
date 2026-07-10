//
// Created by Mr Steven J Baldwin on 04/07/2026.
//

#include "Collision.hpp"
#include <cmath>
#include "../physics/OGCollisionManifold.hpp"


/*  */
bool SphereVolume::intersect(const Ray& ray, RaycastHit& hit) const
{
    glm::vec3 oc = ray.m_origin - m_center;
    float a = glm::dot(ray.m_direction, ray.m_direction);
    float b = 2.0f * glm::dot(oc, ray.m_direction);
    float c = glm::dot(oc, oc) - m_radius * m_radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false;
    }

    float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0) t = (-b + std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0) return false;

    hit.m_distance = t;
    hit.m_position = ray.m_origin + t * ray.m_direction;
    hit.m_normal = glm::normalize(hit.m_position - m_center);
    return true;
}

/* */
bool SphereVolume::intersect(const AABBVolume &aabb) const {
    // Find the point on the AABB closest to the sphere center
    float closestX = std::max(aabb.m_min.x, std::min(m_center.x, aabb.m_max.x));
    float closestY = std::max(aabb.m_min.y, std::min(m_center.y, aabb.m_max.y));
    float closestZ = std::max(aabb.m_min.z, std::min(m_center.z, aabb.m_max.z));

    float distanceSquared = (closestX - m_center.x) * (closestX - m_center.x) +
                           (closestY - m_center.y) * (closestY - m_center.y) +
                           (closestZ - m_center.z) * (closestZ - m_center.z);

    return distanceSquared <= (m_radius * m_radius);
}

/*  */
bool SphereVolume::intersect(const SphereVolume &other) const {
    glm::vec3 dist = m_center - other.m_center;
    float distanceSquared = glm::dot(dist, dist);
    float radiusSum = m_radius + other.m_radius;
    return distanceSquared <= (radiusSum * radiusSum);
}

/*  */
bool SphereVolume::intersect(const PlaneVolume &plane) const {
    float distance = glm::dot(m_center, plane.m_normal) - plane.m_distance;
    return std::abs(distance) <= m_radius;
}

bool SphereVolume::intersect(const OBBVolume &obb) const {
    // Transform the sphere center into the OBB's local space
    glm::vec3 localCenter = glm::vec3(glm::inverse(glm::mat4_cast(obb.m_orientation)) * glm::vec4((m_center - obb.m_center),1.0));

    // Find the closest point on the OBB to the sphere center
    glm::vec3 closestPoint = glm::clamp(localCenter, -obb.m_extents, obb.m_extents);

    // Calculate the distance from the sphere center to the closest point
    glm::vec3 diff = localCenter - closestPoint;
    float distanceSquared = glm::dot(diff, diff);

    return distanceSquared <= (m_radius * m_radius);
}

void SphereVolume::setCenter(glm::vec3 newCenter) {
    m_center = newCenter;
}

void SphereVolume::setRadius(float radius) {
    m_radius = radius;
}

OGCollisionManifold SphereVolume::resolveCollision(IVolume *volume) {
    return OGCollisionManifold();
}

void SphereVolume::transform(const glm::mat4 &transform) {
    m_center = glm::vec3(transform * glm::vec4(m_center, 1.0f));
}

