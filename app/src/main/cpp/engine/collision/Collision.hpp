//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_COLLISION_HPP
#define OXYOUS_2026_COLLISION_HPP

#include "../../includes.hpp"

class PlaneVolume;
class SphereVolume;
class AABBVolume;
class OBBVolume;
class Ray;
class RaycastHit;

class IVolume {
public:
    virtual ~IVolume() = default;
public:
    /* Intersect Ray with Volume */
    virtual bool intersect(const Ray& ray, RaycastHit& hit) const = 0;

    /* Intersect Sphere with Volume */
    [[nodiscard]] virtual bool intersect(const SphereVolume& sphere) const = 0;

    /* Intersect AABB with Volume */
    [[nodiscard]] virtual bool intersect(const AABBVolume& aabb) const = 0;

    /* Intersect OBB with Volume */
    [[nodiscard]] virtual bool intersect(const OBBVolume& obb) const = 0;
};

/* Plane Volume */
class PlaneVolume : public IVolume {
public:
    PlaneVolume(const glm::vec3& normal, float distance) : m_normal(normal), m_distance(distance) {}
    ~PlaneVolume() override = default;
public:
    /* Intersect Ray with Volume */
    bool intersect(const Ray& ray, RaycastHit& hit) const override;

    /* Intersect Sphere with Volume */
    bool intersect(const SphereVolume& sphere) const override;

    /* Intersect AABB with Volume */
    bool intersect(const AABBVolume& aabb) const override;

    /* Intersect OBB with Volume */
    bool intersect(const OBBVolume& obb) const override;
protected:
    glm::vec3 m_normal;
    float m_distance;
};

/* Sphere Volume */
class SphereVolume : public IVolume {
public:
    SphereVolume(const glm::vec3& center, float radius) : m_center(center), m_radius(radius) {}
    ~SphereVolume() override = default;

public:
    /* Intersect Ray with Volume */
    bool intersect(const Ray& ray, RaycastHit& hit) const override;
    /* Intersect Sphere with Volume */
    bool intersect(const SphereVolume& sphere) const override;
    /* Intersect AABB with Volume */
    bool intersect(const AABBVolume& aabb) const override;
    /* Intersect OBB with Volume */
    bool intersect(const OBBVolume& obb) const override;
protected:
    glm::vec3 m_center;
    float m_radius;
};

/* AABB Volume */
class AABBVolume : public IVolume {
public:
    AABBVolume(const glm::vec3& min, const glm::vec3& max) : m_min(min), m_max(max) {}

    ~AABBVolume() override = default;

public:
    /* Intersect Ray with Volume */
    bool intersect(const Ray& ray, RaycastHit& hit) const override;

    /* Intersect Sphere with Volume */
    bool intersect(const SphereVolume& sphere) const override;

    /* Intersect AABB with Volume */
    bool intersect(const AABBVolume& aabb) const override;

    /* Intersect OBB with Volume */
    bool intersect(const OBBVolume& obb) const override;

protected:
    glm::vec3 m_min;
    glm::vec3 m_max;
};

/* OBB Volume */
class OBBVolume : public IVolume {
public:
    OBBVolume(const glm::vec3& center, const glm::vec3& extents, const glm::quat& rotation) : m_center(center), m_extents(extents), m_rotation(rotation) {}

    ~OBBVolume() override = default;
public:
    /* Intersect Ray with Volume */
    bool intersect(const Ray& ray, RaycastHit& hit) const override;

    /* Intersect Sphere with Volume */
    bool intersect(const SphereVolume& sphere) const override;

    /* Intersect AABB with Volume */
    bool intersect(const AABBVolume& aabb) const override;

    /* Intersect OBB with Volume */
    bool intersect(const OBBVolume& obb) const override;

protected:
    glm::vec3 m_center;
    glm::vec3 m_extents;
    glm::quat m_rotation;
};

/*  */
class Ray
{
public:
    Ray(const glm::vec3& origin, const glm::vec3& direction) : m_origin(origin), m_direction(direction) {}
    ~Ray() = default;
public:
    glm::vec3 m_origin;
    glm::vec3 m_direction;
};

class RaycastHit{
public:
    RaycastHit() = default;
    ~RaycastHit() = default;
public:
    glm::vec3 m_position;
    glm::vec3 m_normal;
    float m_distance;
};

#endif //OXYOUS_2026_COLLISION_HPP
