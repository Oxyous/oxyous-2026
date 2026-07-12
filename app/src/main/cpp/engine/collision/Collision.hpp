//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_COLLISION_HPP
#define OXYOUS_2026_COLLISION_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"
#include "engine/entity/OGEntity.hpp"

class PlaneVolume;
class SphereVolume;
class AABBVolume;
class CapsuleVolume;
class OBBVolume;
class Ray;
class RaycastHit;
class Frustum;
class OGCollisionManifold;
class OGEntity;

class IVolume {
public:

    friend class CollisionFactory;

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

    /* Intersect Plane with Volume */
    [[nodiscard]] virtual bool intersect(const PlaneVolume& plane) const = 0;

    /** Determinate Collision with Volume*/
    virtual OGCollisionManifold resolveCollision(IVolume* volume) = 0;

    /** Transform Volume */
    virtual void transform(const glm::mat4& transform) = 0;

    [[nodiscard]] OGEntity* getOwner() const { return m_owner; }
    void setOwner(OGEntity* owner) { m_owner = owner; }

protected:
    OGEntity* m_owner = nullptr;
};

/* Plane Volume */
class PlaneVolume : public IVolume {
public:
    PlaneVolume() = default;
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

    bool intersect(const PlaneVolume& plane) const override;

    virtual OGCollisionManifold resolveCollision(IVolume* volume) override;

    /** Transform Volume*/
    virtual void transform(const glm::mat4& transform) override;

public:
    glm::vec3 m_normal;
    float m_distance;
};

/* Sphere Volume */
class SphereVolume : public IVolume {
public:
    friend class CollisionFactory;

    SphereVolume() = default;
    SphereVolume(const glm::vec3& center, float radius) : m_center(center), m_radius(radius) {}
    ~SphereVolume() override = default;

    [[nodiscard]] glm::vec3 getCenter() const { return m_center; }
    [[nodiscard]] float getRadius() const { return m_radius; }

    void setCenter(glm::vec3 newCenter);

    void setRadius(float radius);

public:
    /* Intersect Ray with Volume */
    bool intersect(const Ray& ray, RaycastHit& hit) const override;
    /* Intersect Sphere with Volume */
    bool intersect(const SphereVolume& sphere) const override;
    /* Intersect AABB with Volume */
    bool intersect(const AABBVolume& aabb) const override;
    /* Intersect OBB with Volume */
    bool intersect(const OBBVolume& obb) const override;
    /* Intersect Plane with Volume */
    bool intersect(const PlaneVolume& obb) const override;
    /* resolve Collision */
    virtual OGCollisionManifold resolveCollision(IVolume* volume) override;
    /** Transform Volume*/
    virtual void transform(const glm::mat4& transform) override;
protected:
    glm::vec3 m_center;
    float m_radius;
};

/** Capsule Collision*/
class CapsuleVolume: public IVolume
{
public:
    friend class CollisionFactory;

    CapsuleVolume() = default;
    CapsuleVolume(const glm::vec3& base, const glm::vec3& top, float radius) : m_base(base), m_top(top), m_radius(radius) {}
    ~CapsuleVolume() override = default;

    [[nodiscard]] glm::vec3 getBase() const { return m_base; }
    [[nodiscard]] glm::vec3 getTop() const { return m_top; }
    [[nodiscard]] float getRadius() const { return m_radius; }

public:
    /** Transform Collision Volume */
    CapsuleVolume transform(const glm::vec3& position, const glm::quat& rotation) const;

    /* Intersect Ray with Volume */
    bool intersect(const Ray& ray, RaycastHit& hit) const override;

    /* Intersect Sphere with Volume */
    bool intersect(const SphereVolume& sphere) const override;

    /* Intersect AABB with Volume */
    bool intersect(const AABBVolume& aabb) const override;

    /* Intersect OBB with Volume */
    bool intersect(const OBBVolume& obb) const override;

    /* Intersect Plane with Volume */
    bool intersect(const PlaneVolume& plane) const override;

    /* resolve Collision */
    virtual OGCollisionManifold resolveCollision(IVolume* volume) override;

    /** Transform Volume*/
    virtual void transform(const glm::mat4& transform) override;
protected:
    glm::vec3 m_base;
    glm::vec3 m_top;
    float m_radius;
};

/* AABB Volume */
class AABBVolume : public IVolume {
public:
    friend class CollisionFactory;

    AABBVolume(const glm::vec3& min, const glm::vec3& max) : m_min(min), m_max(max) {}

    AABBVolume() {
        m_min = glm::vec3(FLT_MAX);
        m_max = glm::vec3(-FLT_MAX);
    }

    ~AABBVolume() override = default;

    /* */
    glm::vec3 getMin() const { return m_min; }

    /* */
    glm::vec3 getMax() const { return m_max; }

    /* Extend Volume by Point */
    void addPoint(const glm::vec3& point) {
        m_min = glm::min(m_min, point);
        m_max = glm::max(m_max, point);
    }

    /* Extend Volume by Volume */
    void addVolume(const AABBVolume& volume) {
        m_min = glm::min(m_min, volume.getMin());
        m_max = glm::max(m_max, volume.getMax());
    }

    glm::vec3 getCentroid() {
        return (m_max + m_min) * 0.5f;
    }

public:
    /* Intersect Ray with Volume */
    bool intersect(const Ray& ray, RaycastHit& hit) const override;

    /* Intersect Sphere with Volume */
    bool intersect(const SphereVolume& sphere) const override;

    /* Intersect AABB with Volume */
    bool intersect(const AABBVolume& aabb) const override;

    /* Intersect OBB with Volume */
    bool intersect(const OBBVolume& obb) const override;

    bool intersect(const PlaneVolume& plane) const override;

    /* resolve Collision */
    virtual OGCollisionManifold resolveCollision(IVolume* volume) override;

    /** Transform Volume*/
    virtual void transform(const glm::mat4& transform) override;
public:
    glm::vec3 m_min;
    glm::vec3 m_max;
};

/* OBB Volume */
class OBBVolume : public IVolume {
public:
    friend class CollisionFactory;
    OBBVolume() = default;
    OBBVolume(const glm::vec3& center, const glm::vec3& extents, const glm::quat& rotation) : m_center(center), m_extents(extents), m_orientation(rotation) {}

    ~OBBVolume() override = default;

    /**/
    glm::quat getOrientation() const { return m_orientation; }

    glm::vec3 getCenter() const { return m_center; }

    glm::vec3 getExtents() const { return m_extents; }

public:
    /* Intersect Ray with Volume */
    bool intersect(const Ray& ray, RaycastHit& hit) const override;

    /* Intersect Sphere with Volume */
    bool intersect(const SphereVolume& sphere) const override;

    /* Intersect AABB with Volume */
    bool intersect(const AABBVolume& aabb) const override;

    /* Intersect OBB with Volume */
    bool intersect(const OBBVolume& obb) const override;

    /** */
    bool intersect(const PlaneVolume& plane) const override;

    /* resolve Collision */
    virtual OGCollisionManifold resolveCollision(IVolume* volume) override;

    /** Transform Volume*/
    virtual void transform(const glm::mat4& transform) override;
public:
    glm::vec3 m_center;
    glm::vec3 m_extents;
    glm::quat m_orientation;
};

/**  */
class Ray
{
public:
    friend class CollisionFactory;

    Ray(const glm::vec3& origin, const glm::vec3& direction) : m_origin(origin), m_direction(direction) {}
    ~Ray() = default;
public:
    glm::vec3 m_origin;
    glm::vec3 m_direction;
};


/**  */
class RaycastHit{
public:
    friend class CollisionFactory;

    RaycastHit() = default;
    ~RaycastHit() = default;
public:
    glm::vec3 m_position;
    glm::vec3 m_normal;
    float m_distance;
};

class Frustum
{
public:
    Frustum() = default;
    ~Frustum() = default;

    /** Initialize frustum planes from a view-projection matrix */
    void update(const glm::mat4& projection, const glm::mat4& view, float nearPlane = 0.1f, float FarPlane = 10000.0f);

    /** Check if an AABB is inside or intersecting the frustum */
    [[nodiscard]] bool intersects(const AABBVolume& aabb) const;

    /** Check if an OBB is inside or intersecting the frustum */
    [[nodiscard]] bool intersects(const OBBVolume& obb) const;

    /** Check if a Sphere is inside or intersecting the frustum */
    [[nodiscard]] bool intersects(const SphereVolume& sphere) const;

    /** Check if a point is inside or intersecting the frustum */
    [[nodiscard]] bool intersects(const glm::vec3& point) const;
public:
    PlaneVolume m_planes[6];
};

#endif //OXYOUS_2026_COLLISION_HPP
