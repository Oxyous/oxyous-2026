//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#include "Collision.hpp"

bool CapsuleVolume::intersect(const Ray& ray, RaycastHit& hit) const {
    // TODO: Implement Ray-Capsule intersection
    return false;
}

bool CapsuleVolume::intersect(const SphereVolume& sphere) const {
    // Find the closest point on the capsule line segment to the sphere center
    glm::vec3 segment = m_top - m_base;
    glm::vec3 toSphere = sphere.getCenter() - m_base;
    float t = glm::dot(toSphere, segment) / glm::dot(segment, segment);
    t = glm::clamp(t, 0.0f, 1.0f);

    glm::vec3 closestPoint = m_base + t * segment;
    glm::vec3 diff = closestPoint - sphere.getCenter();
    float distanceSq = glm::dot(diff, diff);
    float combinedRadius = m_radius + sphere.getRadius();
    return distanceSq <= (combinedRadius * combinedRadius);
}

bool CapsuleVolume::intersect(const AABBVolume& aabb) const {
    // TODO: Implement Capsule-AABB intersection
    return false;
}

bool CapsuleVolume::intersect(const OBBVolume& obb) const {
    // TODO: Implement Capsule-OBB intersection
    return false;
}

bool CapsuleVolume::intersect(const PlaneVolume& plane) const {
    // TODO: Implement Capsule-Plane intersection
    return false;
}

CapsuleVolume CapsuleVolume::transform(const glm::vec3 &position, const glm::quat &rotation) const {
    glm::vec3 newBase = position + (rotation * m_base);
    glm::vec3 newTop = position + (rotation * m_top);
    return CapsuleVolume(newBase, newTop, m_radius);
}

