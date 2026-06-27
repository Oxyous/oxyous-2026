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
        bounds.addPoint(poly.a);
        bounds.addPoint(poly.b);
        bounds.addPoint(poly.c);
        return bounds;
    }

    /* Calculate Polygon Centroid  */
    inline static glm::vec3 PolygonCentroid(const OGPolygon &poly) {
        return (poly.a + poly.b + poly.c) / 3.0f;
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
        glm::vec3 e1 = poly.b - poly.a;
        glm::vec3 e2 = poly.c - poly.a;

        glm::vec3 p = glm::cross(dir, e2);
        double det = glm::dot(e1, p);

        if (std::abs(det) < EPS) {
            return false;
        }

        double invDet = 1.0 / det;
        glm::vec3 s = a - poly.a;
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


};

#endif //OXYOUS_2026_COLLISIONHELPER_HPP
