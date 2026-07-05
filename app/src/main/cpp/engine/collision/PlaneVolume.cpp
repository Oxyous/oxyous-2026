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
    float dist = glm::dot(sphere.getCenter(), m_normal) + m_distance;

    return fabs(dist) <= sphere.getRadius();
}

bool PlaneVolume::intersect(const AABBVolume &aabb) const {
    const auto center = (aabb.getMax() + aabb.getMin()) / 2.0f;
    const auto extents = aabb.getMax() - center;

    float r = extents.x * fabs(glm::dot(m_normal, glm::vec3(1.0f, 0.0f, 0.0f)));

    float dist = glm::dot(center, m_normal) + m_distance;

    if (fabs(dist) <= r) {
        return true;
    }

    return false;
}

bool PlaneVolume::intersect(const OBBVolume &obb) const {
    const auto axisX = obb.getOrientation() * glm::vec3(1.0f, 0.0f, 0.0f);
    const auto axisY = obb.getOrientation() * glm::vec3(0.0f, 1.0f, 0.0f);
    const auto axisZ = obb.getOrientation() * glm::vec3(0.0f, 0.0f, 1.0f);

    float r = obb.getExtents().x * fabs(glm::dot(m_normal, axisX)) +
              obb.getExtents().y * fabs(glm::dot(m_normal, axisY)) +
              obb.getExtents().z * fabs(glm::dot(m_normal, axisZ));

    float dist = glm::dot(obb.getCenter(), m_normal) + m_distance;

    if (fabs(dist) <= r) {
        return true;
    }

    return false;
}

bool PlaneVolume::intersect(const PlaneVolume &plane) const{
    // Two planes intersect if they are not parallel
    float dot = glm::dot(m_normal, plane.m_normal);
    return std::abs(dot) < 1.0f - 1e-6f; // Allow for some numerical error
}
