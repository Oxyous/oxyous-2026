//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_COLLISION_HPP
#define OXYOUS_2026_COLLISION_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"

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

    virtual bool intersect(const Ray& ray, RaycastHit& hit) const = 0;
    [[nodiscard]] virtual bool intersect(const SphereVolume& sphere) const = 0;
    [[nodiscard]] virtual bool intersect(const AABBVolume& aabb) const = 0;
    [[nodiscard]] virtual bool intersect(const OBBVolume& obb) const = 0;
    [[nodiscard]] virtual bool intersect(const PlaneVolume& plane) const = 0;

    virtual OGCollisionManifold resolveCollision(IVolume* volume) = 0;
    virtual void transform(const glm::mat4& transform) = 0;

    [[nodiscard]] OGEntity* getOwner() const { return m_owner; }
    void setOwner(OGEntity* owner) { m_owner = owner; }

    virtual AABBVolume getAABB() const = 0;

protected:
    OGEntity* m_owner = nullptr;
};

class AABBVolume : public IVolume {
public:
    friend class CollisionFactory;
    AABBVolume(const glm::vec3& min, const glm::vec3& max) : m_min(min), m_max(max) {}
    AABBVolume() { m_min = glm::vec3(FLT_MAX); m_max = glm::vec3(-FLT_MAX); }
    ~AABBVolume() override = default;

    glm::vec3 getMin() const { return m_min; }
    glm::vec3 getMax() const { return m_max; }
    void addPoint(const glm::vec3& point) { m_min = glm::min(m_min, point); m_max = glm::max(m_max, point); }
    void addVolume(const AABBVolume& volume) { m_min = glm::min(m_min, volume.getMin()); m_max = glm::max(m_max, volume.getMax()); }
    void addVolume(const struct OGPolygon& poly);
    glm::vec3 getCentroid() const { return (m_max + m_min) * 0.5f; }
    void expand(const glm::vec3& point) { m_min = glm::min(m_min, point); m_max = glm::max(m_max, point); }

    bool intersect(const Ray& ray, RaycastHit& hit) const override;
    bool intersect(const SphereVolume& sphere) const override;
    bool intersect(const AABBVolume& aabb) const override;
    bool intersect(const OBBVolume& obb) const override;
    bool intersect(const PlaneVolume& plane) const override;
    OGCollisionManifold resolveCollision(IVolume* volume) override;
    void transform(const glm::mat4& transform) override;

    AABBVolume getAABB() const override { return *this; }

    glm::vec3 m_min;
    glm::vec3 m_max;
};

class PlaneVolume : public IVolume {
public:
    PlaneVolume() = default;
    PlaneVolume(const glm::vec3& normal, float distance) : m_normal(normal), m_distance(distance) {}
    ~PlaneVolume() override = default;

    bool intersect(const Ray& ray, RaycastHit& hit) const override;
    bool intersect(const SphereVolume& sphere) const override;
    bool intersect(const AABBVolume& aabb) const override;
    bool intersect(const OBBVolume& obb) const override;
    bool intersect(const PlaneVolume& plane) const override;
    OGCollisionManifold resolveCollision(IVolume* volume) override;
    void transform(const glm::mat4& transform) override;

    AABBVolume getAABB() const override {
        return AABBVolume(glm::vec3(-10000.0f), glm::vec3(10000.0f));
    }

    glm::vec3 m_normal;
    float m_distance;
};

class SphereVolume : public IVolume {
public:
    friend class CollisionFactory;
    SphereVolume() = default;
    SphereVolume(const glm::vec3& center, float radius) : m_center(center), m_radius(radius) {}
    ~SphereVolume() override = default;

    [[nodiscard]] glm::vec3 getCenter() const { return m_center; }
    [[nodiscard]] float getRadius() const { return m_radius; }
    void setCenter(glm::vec3 newCenter) { m_center = newCenter; }
    void setRadius(float radius) { m_radius = radius; }

    bool intersect(const Ray& ray, RaycastHit& hit) const override;
    bool intersect(const SphereVolume& sphere) const override;
    bool intersect(const AABBVolume& aabb) const override;
    bool intersect(const OBBVolume& obb) const override;
    bool intersect(const PlaneVolume& plane) const override;
    OGCollisionManifold resolveCollision(IVolume* volume) override;
    void transform(const glm::mat4& transform) override;

    AABBVolume getAABB() const override {
        return AABBVolume(m_center - glm::vec3(m_radius), m_center + glm::vec3(m_radius));
    }

protected:
    glm::vec3 m_center;
    float m_radius;
};

class CapsuleVolume: public IVolume {
public:
    friend class CollisionFactory;
    CapsuleVolume() = default;
    CapsuleVolume(const glm::vec3& base, const glm::vec3& top, float radius) : m_base(base), m_top(top), m_radius(radius) {}
    ~CapsuleVolume() override = default;

    [[nodiscard]] glm::vec3 getBase() const { return m_base; }
    [[nodiscard]] glm::vec3 getTop() const { return m_top; }
    [[nodiscard]] float getRadius() const { return m_radius; }

    CapsuleVolume transform(const glm::vec3& position, const glm::quat& rotation) const;
    bool intersect(const Ray& ray, RaycastHit& hit) const override;
    bool intersect(const SphereVolume& sphere) const override;
    bool intersect(const AABBVolume& aabb) const override;
    bool intersect(const OBBVolume& obb) const override;
    bool intersect(const PlaneVolume& plane) const override;
    OGCollisionManifold resolveCollision(IVolume* volume) override;
    void transform(const glm::mat4& transform) override;

    AABBVolume getAABB() const override {
        return AABBVolume(glm::min(m_base, m_top) - glm::vec3(m_radius), glm::max(m_base, m_top) + glm::vec3(m_radius));
    }

protected:
    glm::vec3 m_base;
    glm::vec3 m_top;
    float m_radius;
};

class OBBVolume : public IVolume {
public:
    friend class CollisionFactory;
    OBBVolume() = default;
    OBBVolume(const glm::vec3& center, const glm::vec3& extents, const glm::quat& rotation) : m_center(center), m_extents(extents), m_orientation(rotation) {}
    ~OBBVolume() override = default;

    glm::quat getOrientation() const { return m_orientation; }
    glm::vec3 getCenter() const { return m_center; }
    glm::vec3 getExtents() const { return m_extents; }

    bool intersect(const Ray& ray, RaycastHit& hit) const override;
    bool intersect(const SphereVolume& sphere) const override;
    bool intersect(const AABBVolume& aabb) const override;
    bool intersect(const OBBVolume& obb) const override;
    bool intersect(const PlaneVolume& plane) const override;
    OGCollisionManifold resolveCollision(IVolume* volume) override;
    void transform(const glm::mat4& transform) override;

    AABBVolume getAABB() const override {
        glm::vec3 corners[8] = {
            {-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1},
            {-1, -1, 1}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 1}
        };
        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_center) * glm::mat4_cast(m_orientation) * glm::scale(glm::mat4(1.0f), m_extents);
        AABBVolume result;
        for (int i = 0; i < 8; i++) {
            result.addPoint(glm::vec3(model * glm::vec4(corners[i], 1.0f)));
        }
        return result;
    }

    glm::vec3 m_center;
    glm::vec3 m_extents;
    glm::quat m_orientation;
};

class Ray {
public:
    friend class CollisionFactory;
    Ray(const glm::vec3& origin, const glm::vec3& direction) : m_origin(origin), m_direction(direction) {}
    glm::vec3 m_origin;
    glm::vec3 m_direction;
};

class RaycastHit {
public:
    friend class CollisionFactory;
    glm::vec3 m_position;
    glm::vec3 m_normal;
    float m_distance;
};

class Frustum {
public:
    void update(const glm::mat4& projection, const glm::mat4& view, float nearPlane = 0.1f, float FarPlane = 10000.0f);
    [[nodiscard]] bool intersects(const AABBVolume& aabb) const;
    [[nodiscard]] bool intersects(const OBBVolume& obb) const;
    [[nodiscard]] bool intersects(const SphereVolume& sphere) const;
    [[nodiscard]] bool intersects(const glm::vec3& point) const;
    PlaneVolume m_planes[6];
};

#endif //OXYOUS_2026_COLLISION_HPP
