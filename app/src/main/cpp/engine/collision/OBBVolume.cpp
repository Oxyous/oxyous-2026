//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#include "Collision.hpp"
#include "../physics/OGCollisionManifold.hpp"
#include "engine/physics/OGPhysicsManager.hpp"
#include "CollisionHelper.hpp"

bool OBBVolume::intersect(const Ray &ray, RaycastHit &hit) const {
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();
    int hitAxis = 0;

    glm::vec3 delta = m_center - ray.m_origin;

    // Test intersection with the 3 planes of the OBB
    for (auto i = 0; i < 3; ++i) {
        glm::vec3 axis = glm::mat3_cast(m_orientation)[i];
        float e = glm::dot(axis, delta);
        float f = glm::dot(ray.m_direction, axis);

        if (std::abs(f) > 0.0001f) {
            float t1 = (e - m_extents[i]) / f;
            float t2 = (e + m_extents[i]) / f;

            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tMin) {
                tMin = t1;
                hitAxis = i;
            }
            if (t2 < tMax) tMax = t2;

            if (tMin > tMax) return false;
        } else {
            if (-e - m_extents[i] > 0.0f || -e + m_extents[i] < 0.0f) return false;
        }
    }

    if (tMax < 0) return false;

    hit.m_distance = tMin;
    hit.m_position = ray.m_origin + ray.m_direction * tMin;

    glm::vec3 normal = glm::mat3_cast(m_orientation)[hitAxis];
    hit.m_normal = glm::dot(ray.m_direction, normal) < 0 ? normal : -normal;

    return true;
}

bool OBBVolume::intersect(const SphereVolume &sphere) const {
    glm::vec3 delta = sphere.getCenter() - m_center;
    glm::mat3 rotation = glm::mat3_cast(m_orientation);
    glm::vec3 closestPoint = m_center;

    for (int i = 0; i < 3; ++i) {
        glm::vec3 axis = rotation[i];
        float dist = glm::dot(delta, axis);

        if (dist > m_extents[i]) dist = m_extents[i];
        if (dist < -m_extents[i]) dist = -m_extents[i];

        closestPoint += axis * dist;
    }

    float distanceSq = glm::dot(sphere.getCenter() - closestPoint, sphere.getCenter() - closestPoint);
    return distanceSq <= (sphere.getRadius() * sphere.getRadius());
}

bool OBBVolume::intersect(const AABBVolume &aabb) const {


    return false;
}

bool OBBVolume::intersect(const OBBVolume &obb) const {
    return false;
}

bool OBBVolume::intersect(const PlaneVolume &plane) const {
    return false;
}

OGCollisionManifold OBBVolume::resolveCollision(IVolume *volume) {

    if (dynamic_cast<OBBVolume*>(volume)) {
        return CollisionHelper::resolveCollision(*this, *dynamic_cast<OBBVolume*>(volume));
    }

    if (dynamic_cast<SphereVolume*>(volume)) {
        return CollisionHelper::resolveCollision(*this, *dynamic_cast<SphereVolume*>(volume));
    }
/*
    if (dynamic_cast<AABBVolume*>(volume)) {
        return CollisionHelper::resolveCollision(*this, *dynamic_cast<AABBVolume*>(volume));
    }*/

    if (dynamic_cast<PlaneVolume*>(volume)) {
        return CollisionHelper::resolveCollision(*this, *dynamic_cast<PlaneVolume*>(volume));
    }

    if (dynamic_cast<CapsuleVolume*>(volume)) {
        return CollisionHelper::resolveCollision(*dynamic_cast<CapsuleVolume*>(volume), *this);
    }

    return OGCollisionManifold();
}

void OBBVolume::transform(const glm::mat4 &transform) {
    m_center = transform[3];
    m_orientation = glm::quat_cast(transform);
}
