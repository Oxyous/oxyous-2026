//
// Created by Mr Steven J Baldwin on 06/07/2026.
//

#ifndef OXYOUS_2026_MATHHELPER_HPP
#define OXYOUS_2026_MATHHELPER_HPP

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "external/glm/vec2.hpp"
#include "DataStructures.hpp"
#include "external/glm/vec3.hpp"

class Math {
public:
    static constexpr float EPS = 1e-9;

    static double cross2d(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    /** Returns true if point p lies on the line segment ab. */
    static bool onSegment2d(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &p) {
        return std::abs(cross2d(a, b, p)) < EPS &&
               p.x >= std::min(a.x, b.x) - EPS && p.x <= std::max(a.x, b.x) + EPS &&
               p.y >= std::min(a.y, b.y) - EPS && p.y <= std::max(a.y, b.y) + EPS;
    }

    /** Returns true if line segment ab intersects with line segment cd. */
    static bool
    intersect2d(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c, const glm::vec2 &d) {
        double d1 = cross2d(c, d, a);
        double d2 = cross2d(c, d, b);
        double d3 = cross2d(a, b, c);
        double d4 = cross2d(a, b, d);

        if (((d1 > EPS && d2 < -EPS) || (d1 < -EPS && d2 > EPS)) &&
            ((d3 > EPS && d4 < -EPS) || (d3 < -EPS && d4 > EPS)))
            return true;

        return onSegment2d(c, d, a) || onSegment2d(c, d, b) || onSegment2d(a, b, c) ||
               onSegment2d(a, b, d);
    }

    /** Returns the intersection point of line segments ab and cd. Assumes they intersect. */
    static glm::vec2 getIntersection2d(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c,
                                       const glm::vec2 &d) {
        double s1_x = b.x - a.x;
        double s1_y = b.y - a.y;
        double s2_x = d.x - c.x;
        double s2_y = d.y - c.y;

        double s, t;
        s = (-s1_y * (a.x - c.x) + s1_x * (a.y - c.y)) / (-s2_x * s1_y + s1_x * s2_y);
        t = (s2_x * (a.y - c.y) - s2_y * (a.x - c.x)) / (-s2_x * s1_y + s1_x * s2_y);

        return glm::vec2(a.x + (t * s1_x), a.y + (t * s1_y));
    }

    /** Returns the shortest distance from point p to line segment ab. */
    static float distToSegment2d(const glm::vec2 &p, const glm::vec2 &a, const glm::vec2 &b) {
        float l2 = std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2);
        if (l2 == 0.0f) return std::sqrt(std::pow(p.x - a.x, 2) + std::pow(p.y - a.y, 2));
        float t = std::max(0.0f, std::min(1.0f,
                                          ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) /
                                          l2));
        glm::vec2 projection = glm::vec2(
                a.x + t * (b.x - a.x),
                a.y + t * (b.y - a.y)
        );
        return std::sqrt(std::pow(p.x - projection.x, 2) + std::pow(p.y - projection.y, 2));
    }

    /** Returns true if the point is strictly inside the triangle formed by the first 3 vertices of the polygon on the XZ plane. */
    static bool pointStrictlyInsideTriangleXZ(const glm::vec3 &point, const OGPolygon &polygon) {
        glm::vec2 P = {point.x, point.z};
        glm::vec2 A = {polygon.vertices[0].x, polygon.vertices[0].z};
        glm::vec2 B = {polygon.vertices[1].x, polygon.vertices[1].z};
        glm::vec2 C = {polygon.vertices[2].x, polygon.vertices[2].z};

        double c1 = cross2d(A, B, P);
        double c2 = cross2d(B, C, P);
        double c3 = cross2d(C, A, P);

        bool hasNeg = (c1 < -EPS) || (c2 < -EPS) || (c3 < -EPS);
        bool hasPos = (c1 > EPS) || (c2 > EPS) || (c3 > EPS);

        if (!(hasNeg && hasPos)) {
            if (std::abs(c1) <= EPS || std::abs(c2) <= EPS || std::abs(c3) <= EPS) {
                return false;
            }
            return true;
        }

        return false;
    }

    /**  */
    static bool samePointXZ(const glm::vec3 &a, const glm::vec3 &b, double eps = 1e-6) {
        return std::abs(a.x - b.x) <= eps && std::abs(a.z - b.z) <= eps;
    }

    /** Point On Polygon Boundary Check */
    static bool pointOnPolygonBoundaryXZ(const glm::vec3 &p, const OGPolygon &polygon) {
        glm::vec2 P = {p.x, p.z};
        glm::vec2 A = {polygon.vertices[0].x, polygon.vertices[0].z};
        glm::vec2 B = {polygon.vertices[1].x, polygon.vertices[1].z};
        glm::vec2 C = {polygon.vertices[2].x, polygon.vertices[2].z};

        return onSegment2d(A, B, P) || onSegment2d(B, C, P) || onSegment2d(C, A, P);
    }

    /** Point is on a polygon vertex */
    static bool isPolygonVertex(const glm::vec3 &p, const OGPolygon &polygon) {
        return samePointXZ(p, polygon.vertices[0]) || samePointXZ(p, polygon.vertices[1]) ||
               samePointXZ(p, polygon.vertices[2]);
    }

    static int orient2(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c) {
        double v = cross2d(a, b, c);
        if (v > EPS) return 1;
        if (v < -EPS) return -1;
        return 0;
    }

    /** Proper intersection = crossing through interiors, not merely touching at endpoints. */
    static bool segmentsProperlyIntersect2(const glm::vec2 &p1, const glm::vec2 &q1,
                                           const glm::vec2 &p2, const glm::vec2 &q2) {
        int o1 = orient2(p1, q1, p2);
        int o2 = orient2(p1, q1, q2);
        int o3 = orient2(p2, q2, p1);
        int o4 = orient2(p2, q2, q1);
        return (o1 * o2 < 0) && (o3 * o4 < 0);
    }

    /** */
    static bool
    segmentBlockedByPolygonXZ(const glm::vec3 &p0, const glm::vec3 &p1, const OGPolygon &polygon) {
        if (pointStrictlyInsideTriangleXZ(p0, polygon) ||
            pointStrictlyInsideTriangleXZ(p1, polygon)) {
            return true;
        }

        glm::vec2 A = {polygon.vertices[0].x, polygon.vertices[0].z};
        glm::vec2 B = {polygon.vertices[1].x, polygon.vertices[1].z};
        glm::vec2 C = {polygon.vertices[2].x, polygon.vertices[2].z};
        glm::vec2 P0 = {p0.x, p0.z};
        glm::vec2 P1 = {p1.x, p1.z};

        if (segmentsProperlyIntersect2(P0, P1, A, B)) return true;
        if (segmentsProperlyIntersect2(P0, P1, B, C)) return true;
        if (segmentsProperlyIntersect2(P0, P1, C, A)) return true;

        auto touchesBoundary = [&](const glm::vec2 &P) -> bool {
            glm::vec3 tmp{P.x, 0.0, P.y};
            if (pointOnPolygonBoundaryXZ(tmp, polygon) && !isPolygonVertex(tmp, polygon)) {
                return true;
            }
            return false;
        };

        if (touchesBoundary(P0) || touchesBoundary(P1)) return true;

        glm::vec3 mid{0.5f * (p0.x + p1.x), 0.5f * (p0.y + p1.y), 0.5f * (p0.z + p1.z)};
        if (pointStrictlyInsideTriangleXZ(mid, polygon)) return true;

        return false;
    }

    /** Segment is Clear XZ */
    static bool segmentIsClearXZ(const glm::vec3 &a, const glm::vec3 &b,
                                 const std::vector<OGPolygon> &polygons) {
        for (const auto &poly: polygons) {
            if (segmentBlockedByPolygonXZ(a, b, poly)) {
                return false;
            }
        }
        return true;
    }

    static void
    addUniquePoint(std::vector<glm::vec3>& points, const glm::vec3 &p, double eps = 1e-6) {
        for (const auto &existing: points) {
            if (samePointXZ(existing, p, eps)) {
                return;
            }
        }
        points.push_back(p);
    }

    inline static bool isClose(const float &a, const float &b) {
        return (fabsf(a - b) <= 0.0001f);
    }
    /**
    static glm::quat addScaledVector(const glm::vec3& v, const glm::quat& q, float scale) {
        glm::quat scaled = glm::quat(v.x * scale, v.y * scale, v.z * scaled, 0.0f);
        scaled *= q;

        float w += scaled * 0.5f;
        float x += scaled * 0.5f;
        float y += scaled * 0.5f;
        float z += scaled * 0.5f;

        return glm::quat(x,y,z,w);
    }*/
};

#endif //OXYOUS_2026_MATHHELPER_HPP
