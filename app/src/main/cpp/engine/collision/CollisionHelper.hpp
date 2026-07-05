//
// Created by Mr Steven J Baldwin on 26/06/2026.
//

#ifndef OXYOUS_2026_COLLISIONHELPER_HPP
#define OXYOUS_2026_COLLISIONHELPER_HPP

#include "Collision.hpp"

class CollisionHelper {
private:
    static constexpr double EPS = 1e-8;
public:
    /* Calculate Polygon AABB Volume*/
    inline static AABBVolume PolygonBounds(const OGPolygon &poly) {
        AABBVolume bounds{};
        bounds.addPoint(poly.vertices[0]);
        bounds.addPoint(poly.vertices[1]);
        bounds.addPoint(poly.vertices[2]);
        return bounds;
    }

    /* Calculate Polygon Centroid  */
    inline static glm::vec3 PolygonCentroid(const OGPolygon &poly) {
        return (poly.vertices[0] + poly.vertices[1] + poly.vertices[2]) / 3.0f;
    }

    /* Segment Intersects AABB Volume*/
    inline static bool
    segmentIntersectsAabb(const glm::vec3 &a, const glm::vec3 &b, const AABBVolume &aabb) {
        glm::vec3 d = b - a;
        double tmin = 0.0;
        double tmax = 1.0;

        auto update = [&](double start, double dir, double minv, double maxv) -> bool {
            if (std::abs(dir) < EPS) {
                return start >= minv - EPS && start <= maxv + EPS;
            }

            double invD = 1.0 / dir;
            double t1 = (minv - start) * invD;
            double t2 = (maxv - start) * invD;
            if (t1 > t2) std::swap(t1, t2);

            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            return tmin <= tmax + EPS;
        };

        if (!update(a.x, d.x, aabb.getMin().x, aabb.getMax().x)) return false;
        if (!update(a.y, d.y, aabb.getMin().y, aabb.getMax().y)) return false;
        if (!update(a.z, d.z, aabb.getMin().z, aabb.getMax().z)) return false;

        return true;
    }

    /* Segment Intersects Polygon Volume*/
    inline static bool
    segmentIntersectsPolygon(const glm::vec3 &a, const glm::vec3 &b, const OGPolygon &poly,
                             double &_t) {
        glm::vec3 dir = b - a;
        glm::vec3 e1 = poly.vertices[1] - poly.vertices[0];
        glm::vec3 e2 = poly.vertices[2] - poly.vertices[0];

        glm::vec3 p = glm::cross(dir, e2);
        double det = glm::dot(e1, p);

        if (std::abs(det) < EPS) {
            return false;
        }

        double invDet = 1.0 / det;
        glm::vec3 s = a - poly.vertices[0];
        double u = glm::dot(s, p) * invDet;
        if (u < 0.0 || u > 1.0) {
            return false;
        }

        glm::vec3 q = glm::cross(s, e1);
        double v = glm::dot(dir, q) * invDet;
        if (v < 0.0 || u + v > 1.0) {
            return false;
        }

        double t = glm::dot(e2, q) * invDet;

        if (t < -EPS || t > 1.0 + EPS) return false;

        _t = t;

        return true;
    }

    /* */
    inline static PlaneVolume getPolygonPlane(OGPolygon& polygon)
    {
        PlaneVolume plane;
        glm::vec3 ab = polygon.vertices[1] - polygon.vertices[0];
        glm::vec3 ac = polygon.vertices[2] - polygon.vertices[0];
        plane.m_normal = glm::normalize(glm::cross(ab, ac));
        plane.m_distance = glm::dot(plane.m_normal, polygon.vertices[0]);
        return plane;
    }

    /* */
    inline static bool resolvePolygonSphereCollision(OGPolygon &polygon, const SphereVolume& sphereVolume, OGContact& contact)
    {
        PlaneVolume plane = getPolygonPlane(polygon);
        float distance = glm::dot(sphereVolume.getCenter(), plane.m_normal) - plane.m_distance;

        if (std::abs(distance) > sphereVolume.getRadius()) {
            return false;
        }

        glm::vec3 project = sphereVolume.getCenter() - plane.m_normal * distance;

        // Barycentric coordinates check to see if 'project' is inside the triangle
        glm::vec3 v0 = polygon.vertices[1] - polygon.vertices[0];
        glm::vec3 v1 = polygon.vertices[2] - polygon.vertices[0];
        glm::vec3 v2 = project - polygon.vertices[0];

        float d00 = glm::dot(v0, v0);
        float d01 = glm::dot(v0, v1);
        float d11 = glm::dot(v1, v1);
        float d20 = glm::dot(v2, v0);
        float d21 = glm::dot(v2, v1);
        float denom = d00 * d11 - d01 * d01;

        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;

        if (u < 0.0f || v < 0.0f || w < 0.0f) return false;

        contact.normal = plane.m_normal;
        contact.depth = sphereVolume.getRadius() - std::abs(distance);
        contact.hitPoint = project;

        return true;
    }
};

#endif //OXYOUS_2026_COLLISIONHELPER_HPP
