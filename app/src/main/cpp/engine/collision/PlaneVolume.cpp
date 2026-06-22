//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#include "../../includes.hpp"
#include "Collision.hpp"

bool PlaneVolume::intersect(const Ray &ray, RaycastHit &hit) const {
    float numerator = glm::dot(ray.m_origin, m_normal) + m_distance;
    float denom = glm::dot(ray.m_direction, m_normal);

    float t = -(numerator / denom);

    if (t > 0.0f) {
        hit.m_position = ray.m_origin + ray.m_direction * t;
        hit.m_normal = m_normal;
        hit.m_distance = t;
        return true;
    }

    return false;
}

bool PlaneVolume::intersect(const SphereVolume &sphere) const {
    return false;
}

bool PlaneVolume::intersect(const AABBVolume &aabb) const {
    return false;
}

bool PlaneVolume::intersect(const OBBVolume &obb) const {
    return false;
}
