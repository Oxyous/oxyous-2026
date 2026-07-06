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
    [[nodiscard]] inline static AABBVolume PolygonBounds(const OGPolygon &poly) {
        AABBVolume bounds{};
        bounds.addPoint(poly.vertices[0]);
        bounds.addPoint(poly.vertices[1]);
        bounds.addPoint(poly.vertices[2]);
        return bounds;
    }

    /* Calculate Polygon Centroid  */
    [[nodiscard]] inline static glm::vec3 PolygonCentroid(const OGPolygon &poly) {
        return (poly.vertices[0] + poly.vertices[1] + poly.vertices[2]) / 3.0f;
    }

    /* Segment Intersects AABB Volume*/
    [[nodiscard]] inline static bool
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
    [[nodiscard]] inline static bool
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

    /* Closest Point On Triangle*/
    [[nodiscard]] inline static glm::vec3
    ClosestPointOnTriangle(const glm::vec3 &point, const glm::vec3 &a, const glm::vec3 &b,
                           const glm::vec3 &c) {
        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        glm::vec3 ap = point - a;

        float d1 = glm::dot(ab, ap);
        float d2 = glm::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f)
            return a;

        glm::vec3 bp = point - b;
        float d3 = glm::dot(ab, bp);
        float d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
            return b;

        float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            float v = d1 / (d1 - d3);
            return a + ab * v;
        }

        glm::vec3 cp = point - c;
        float d5 = glm::dot(ab, cp);
        float d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
            return c;

        float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            float w = d2 / (d2 - d6);
            return a + ac * w;
        }

        float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + (c - b) * w;
        }

        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;

        return a + ab * v + ac * w;
    }

    /* */
    [[nodiscard]] inline static PlaneVolume getPolygonPlane(const OGPolygon &polygon) {
        PlaneVolume plane;
        glm::vec3 ab = polygon.vertices[1] - polygon.vertices[0];
        glm::vec3 ac = polygon.vertices[2] - polygon.vertices[0];
        plane.m_normal = glm::normalize(glm::cross(ab, ac));
        plane.m_distance = glm::dot(plane.m_normal, polygon.vertices[0]);
        return plane;
    }

    /* */
    [[nodiscard]] inline static bool
    resolvePolygonSphereCollision(const OGPolygon &polygon, const SphereVolume &sphereVolume,
                                  OGContact &contact) {
        glm::vec3 closestToSphere = ClosestPointOnTriangle(sphereVolume.getCenter(),
                                                           polygon.vertices[0], polygon.vertices[1],
                                                           polygon.vertices[2]);
        glm::vec3 sphereToClosest = closestToSphere - sphereVolume.getCenter();

        float d2 = glm::dot(sphereToClosest, sphereToClosest);

        if (d2 < sphereVolume.getRadius() * sphereVolume.getRadius()) {
            float d = sqrt(d2);
            contact.hitPoint = closestToSphere;
            contact.normal = (d > 0.0f) ? -sphereToClosest / d : -getPolygonPlane(polygon).m_normal;
            contact.depth = sphereVolume.getRadius() - d;
            return true;
        }

        return false;
    }

    [[nodiscard]] inline static bool
    resolvePolygonRay(const OGPolygon &polygon, const glm::vec3 &origin, const glm::vec3 direction,
                      OGContact &hitContact) {
        constexpr float ESP = 0.0000001f;

        glm::vec3 edge1 = polygon.vertices[1] - polygon.vertices[0];
        glm::vec3 edge2 = polygon.vertices[2] - polygon.vertices[0];

        glm::vec3 h = glm::cross(direction, edge2);
        float a = glm::dot(edge1, h);

        if (fabs(a) < ESP)
            return false;

        float f = 1.0f / a;
        glm::vec3 s = origin - polygon.vertices[0];
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        glm::vec3 q = glm::cross(s, edge1);

        float v = f * glm::dot(direction, q);

        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        double t = f * glm::dot(edge2, q);

        if (t > ESP) {
            hitContact.depth = t;
            hitContact.hitPoint = origin + direction * (float)t;
            hitContact.normal = glm::normalize(h);
            return true;
        }

        return false;
    }
};

#endif //OXYOUS_2026_COLLISIONHELPER_HPP
