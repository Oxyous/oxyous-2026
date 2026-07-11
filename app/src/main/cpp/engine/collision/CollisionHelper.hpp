//
// Created by Mr Steven J Baldwin on 26/06/2026.
//

#ifndef OXYOUS_2026_COLLISIONHELPER_HPP
#define OXYOUS_2026_COLLISIONHELPER_HPP

#include "Collision.hpp"
#include "../physics/OGCollisionManifold.hpp"

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
    resolvePolygonCapsuleCollision(const OGPolygon &polygon, const CapsuleVolume &capsule,
                                   OGContact &contact) {
        glm::vec3 closestToCapsule = ClosestPointOnTriangle(capsule.getBase(),
                                                            polygon.vertices[0],
                                                            polygon.vertices[1],
                                                            polygon.vertices[2]);
        glm::vec3 capsuleToClosest = closestToCapsule - capsule.getBase();

        float d2 = glm::dot(capsuleToClosest, capsuleToClosest);

        if (d2 < capsule.getRadius() * capsule.getRadius()) {
            float d = sqrt(d2);
            contact.hitPoint = closestToCapsule;
            contact.normal = (d > 0.0f) ? -capsuleToClosest / d : -getPolygonPlane(
                    polygon).m_normal;
            contact.depth = capsule.getRadius() - d;
            return true;
        }

        return false;
    }

    [[nodiscard]] inline static bool
    resolvePolygonObbCollision(const OGPolygon &polygon, const OBBVolume &obb, OGContact &contact) {
        // Get the closest point on the polygon to the OBB center
        glm::vec3 closestToObb = ClosestPointOnTriangle(obb.getCenter(),
                                                        polygon.vertices[0],
                                                        polygon.vertices[1],
                                                        polygon.vertices[2]);
        glm::vec3 obbToClosest = closestToObb - obb.getCenter();

        // Check if the closest point is inside the OBB
        if (pointInsideOBB(obb, closestToObb)) {
            float d2 = glm::dot(obbToClosest, obbToClosest);
            float obbRadius = glm::length(obb.getExtents());

            if (d2 < obbRadius * obbRadius) {
                float d = sqrt(d2);
                contact.hitPoint = closestToObb;
                PlaneVolume plane = getPolygonPlane(polygon);
                contact.normal =
                glm::dot(plane.m_normal,
                obb.getCenter() - polygon.vertices[0]) > 0.0f ? plane.m_normal : -plane.m_normal;
                contact.depth = obbRadius - d;
                return true;
            }
        }

        return false;
    }

    /** Point in Triangle Polygon*/
    [[nodiscard]] inline static bool
    pointInPolygon(const glm::vec3 &point, const OGPolygon &polygon) {
        return glm::dot(
                glm::cross(polygon.vertices[2] - polygon.vertices[0], point - polygon.vertices[0]),
                polygon.normal) >= 0 &&
               glm::dot(glm::cross(polygon.vertices[0] - polygon.vertices[1],
                                   point - polygon.vertices[1]), polygon.normal) >= 0 &&
               glm::dot(glm::cross(polygon.vertices[1] - polygon.vertices[2],
                                   point - polygon.vertices[2]), polygon.normal) >= 0;
    }

    /** Sphere Projection cast*/
    [[nodiscard]] inline static bool
    sphereCastPolygon(const OGPolygon &polygon, const glm::vec3 sphereOrigin, float radius,
                      const glm::vec3 &direction, float maxDistance, OGContact &hitResult) {
        glm::vec3 start = sphereOrigin;

        float startDis = glm::dot(start - polygon.vertices[0], polygon.normal);
        float demon = glm::dot(direction, polygon.normal);

        if (std::abs(demon) < EPS) {
            return false;
        }

        float t = (radius - startDis) / demon;

        /** Valid hit */
        if (t >= 0 && t <= maxDistance) {
            glm::vec3 impactCenter = sphereOrigin + direction * t;

            glm::vec3 contactPoint = impactCenter - polygon.normal * radius;

            if (pointInPolygon(glm::vec3(contactPoint), polygon)) {
                hitResult.hitPoint = contactPoint;
                hitResult.normal = polygon.normal;
                hitResult.depth = t;
                return true;
            }
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
            hitContact.hitPoint = origin + direction * (float) t;
            hitContact.normal = glm::normalize(h);
            return true;
        }

        return false;
    }

    /** Get Projected Range STA*/
    [[nodiscard]]
    inline static glm::vec2
    getPolygonProjectedRange(const OGPolygon &polygon, const glm::vec3 &axis, float &min,
                             float &max) {
        min = max = glm::dot(polygon.vertices[0], axis);
        for (int i = 1; i < 3; ++i) {
            float projection = glm::dot(polygon.vertices[i], axis);
            if (projection < min) {
                min = projection;
            }
            if (projection > max) {
                max = projection;
            }
        }
        return glm::vec2(min, max);
    }

    /** Project SAT range for OBB */
    [[nodiscard]]
    inline static glm::vec2 getProjectRangeOBB(const OBBVolume &obb, const glm::vec3 &axis) {
        glm::vec2 range(FLT_MAX, -FLT_MAX);
        std::vector<glm::vec3> vertices;

        computeObbVertices(obb, vertices);

        for (int i = 0; i < 8; i++) {
            float projection = glm::dot(vertices[i], axis);
            if (projection < range.x) {
                range.x = projection;
            }
            if (projection > range.y) {
                range.y = projection;
            }
        }
        return range;
    }

    /** Test in point lies inside OBB */
    inline static bool pointInsideOBB(const OBBVolume &obb, const glm::vec3 &point) {
        auto direction = point - obb.getCenter();

        for (auto i = 0; i < 3; i++) {
            auto axis = glm::mat3_cast(obb.getOrientation())[i];
            auto projection = glm::dot(direction, axis);
            if (projection < -obb.getExtents()[i] || projection > obb.getExtents()[i]) {
                return false;
            }
        }

        return true;
    }

    /** Extract OBB Vertices */
    inline static void
    computeObbVertices(const OBBVolume &obb, std::vector<glm::vec3> &verticesOut) {
        verticesOut.resize(8);
        glm::vec3 center = obb.getCenter();
        glm::vec3 extents = obb.getExtents();

        auto X = glm::mat3_cast(obb.getOrientation())[0];
        auto Y = glm::mat3_cast(obb.getOrientation())[1];
        auto Z = glm::mat3_cast(obb.getOrientation())[2];

        verticesOut[0] = glm::vec3(center + X * extents.x + Y * extents.y + Z * extents.z);
        verticesOut[1] = glm::vec3(center - X * extents.x + Y * extents.y + Z * extents.z);
        verticesOut[2] = glm::vec3(center + X * extents.x - Y * extents.y + Z * extents.z);
        verticesOut[3] = glm::vec3(center - X * extents.x - Y * extents.y + Z * extents.z);
        verticesOut[4] = glm::vec3(center - X * extents.x - Y * extents.y - Z * extents.z);
        verticesOut[5] = glm::vec3(center + X * extents.x - Y * extents.y - Z * extents.z);
        verticesOut[6] = glm::vec3(center - X * extents.x + Y * extents.y - Z * extents.z);
        verticesOut[7] = glm::vec3(center + X * extents.x + Y * extents.y - Z * extents.z);
    }

    /** Extract OBB Edges */
    inline static void computeObbEdges(const OBBVolume &obb, std::vector<OGSegment> &edges) {
        edges.resize(12);

        std::vector<glm::vec3> vertices;
        computeObbVertices(obb, vertices);

        int indices[][2] = {
                {1, 6},
                {6, 3},
                {6, 4},
                {2, 7},
                {2, 5},
                {2, 0},
                {0, 1},
                {0, 3},
                {7, 1},
                {7, 4},
                {4, 5},
                {5, 3}
        };

        for (auto i = 0; i < 12; i++) {
            edges[i] = OGSegment{
                    vertices[indices[i][0]],
                    vertices[indices[i][1]]
            };
        }
    }

    /** Extract OBB planes */
    inline static void
    computeObbPlanes(const OBBVolume &obb, std::vector<PlaneVolume> &planes) {
        planes.resize(6);

        auto X = glm::mat3_cast(obb.getOrientation())[0];
        auto Y = glm::mat3_cast(obb.getOrientation())[1];
        auto Z = glm::mat3_cast(obb.getOrientation())[2];

        planes[0] = PlaneVolume{X, glm::dot(X, obb.getCenter() + X * obb.getExtents().x)};
        planes[1] = PlaneVolume{-X, glm::dot(-X, obb.getCenter() - X * obb.getExtents().x)};
        planes[2] = PlaneVolume{Y, glm::dot(Y, obb.getCenter() + Y * obb.getExtents().y)};
        planes[3] = PlaneVolume{-Y, glm::dot(-Y, obb.getCenter() - Y * obb.getExtents().y)};
        planes[4] = PlaneVolume{Z, glm::dot(Z, obb.getCenter() + Z * obb.getExtents().z)};
        planes[5] = PlaneVolume{-Z, glm::dot(-Z, obb.getCenter() - Z * obb.getExtents().z)};
    }

    /** Clip a segment by plane */
    inline static bool
    clipSegmentPlane(const OGSegment &segment, const PlaneVolume &plane, glm::vec3 &intersection) {
        glm::vec3 ab = segment.end - segment.start;
        float denom = glm::dot(plane.m_normal, ab);

        if (std::abs(denom) > EPS) {
            float t = (plane.m_distance - glm::dot(plane.m_normal, segment.start)) / denom;
            if (t >= 0.0f && t <= 1.0f) {
                intersection = segment.start + t * ab;
                return true;
            }
        }

        return false;
    }

    /** Clip OBB Edges */
    inline static void clipEdgesOBB(const OBBVolume &obb, const std::vector<OGSegment> &edges,
                                    std::vector<glm::vec3> &pointsOut) {
        std::vector<PlaneVolume> obbPlanes;
        computeObbPlanes(obb, obbPlanes);
        glm::vec3 intersection{};

        for (auto i = 0; i < obbPlanes.size(); i++) {
            for (auto j = 0; j < edges.size(); ++j) {
                if (clipSegmentPlane(edges[j], obbPlanes[i], intersection)) {
                    if (pointInsideOBB(obb, intersection)) {
                        pointsOut.push_back(intersection);
                    }
                }
            }
        }
    }

    /** Calculate Penetration Depth of Two OBBs*/
    inline static float
    penetrationDepth(const OBBVolume &obbA, const OBBVolume &obbB, const glm::vec3 &axis,
                     bool *shouldFlip) {
        auto projA = getProjectRangeOBB(obbA, axis);
        auto projB = getProjectRangeOBB(obbB, axis);

        if (projB.x > projA.y || projA.x > projB.y) {
            return 0.0f;
        }

        auto lenA = projA.y - projA.x;
        auto lenB = projB.y - projB.x;

        auto min = fminf(projA.x, projB.x);
        auto max = fmaxf(projA.y, projB.y);

        auto distance = max - min;

        if (shouldFlip) {
            *shouldFlip = (projB.x < projA.x);
        }

        return (lenA + lenB) - distance;
    }

    /** Resolve Collision Between to OBB volumes */
    inline static OGCollisionManifold
    resolveCollision(const OBBVolume &obbA, const OBBVolume &obbB) {
        OGCollisionManifold manifold{};

        auto aX = glm::mat3_cast(obbA.getOrientation())[0];
        auto aY = glm::mat3_cast(obbA.getOrientation())[1];
        auto aZ = glm::mat3_cast(obbA.getOrientation())[2];

        auto bX = glm::mat3_cast(obbB.getOrientation())[0];
        auto bY = glm::mat3_cast(obbB.getOrientation())[1];
        auto bZ = glm::mat3_cast(obbB.getOrientation())[2];

        glm::vec3 tests[15] = {
                aX,
                aY,
                aZ,
                bX,
                bY,
                bZ
        };

        glm::vec3 aAxes[] = {aX, aY, aZ};
        glm::vec3 bAxes[] = {bX, bY, bZ};

        for (auto i = 0; i < 3; ++i) {
            for (auto j = 0; j < 3; ++j) {
                tests[6 + i * 3 + j] = glm::cross(aAxes[i], bAxes[j]);
            }
        }

        glm::vec3 bestAxis = glm::vec3(0.0f);
        bool shouldFlip = false;
        bool found = false;

        for (auto i = 0; i < 15; ++i) {
            float magSq = glm::dot(tests[i], tests[i]);
            if (magSq < 1e-6f) {
                continue;
            }

            glm::vec3 axis = tests[i] * (1.0f / sqrtf(magSq));
            bool currentShouldFlip = false;
            float depth = CollisionHelper::penetrationDepth(obbA, obbB, axis, &currentShouldFlip);

            if (depth <= 0.0f) {
                return OGCollisionManifold();
            }

            if (depth < manifold.m_depth) {
                manifold.m_depth = depth;
                bestAxis = axis;
                shouldFlip = currentShouldFlip;
                found = true;
            }
        }

        /** Exit if no normal found */
        if (!found) {
            return OGCollisionManifold();
        }

        glm::vec3 axis = shouldFlip ? -bestAxis : bestAxis;
        manifold.m_normal = axis;

        std::vector<glm::vec3> c1, c2;
        std::vector<OGSegment> e1, e2;
        CollisionHelper::computeObbEdges(obbA, e1);
        CollisionHelper::computeObbEdges(obbB, e2);

        CollisionHelper::clipEdgesOBB(obbA, e2, c1);
        CollisionHelper::clipEdgesOBB(obbB, e1, c2);

        manifold.m_contacts.clear();
        manifold.m_contacts.reserve(c1.size() + c2.size());
        manifold.m_contacts.insert(manifold.m_contacts.end(), c1.begin(), c1.end());
        manifold.m_contacts.insert(manifold.m_contacts.end(), c2.begin(), c2.end());

        if (manifold.m_contacts.empty()) {
            manifold.m_contacts.push_back((obbA.getCenter() + obbB.getCenter()) * 0.5f);
        }

        /** Project contact points onto the reference plane to ensure they are on the surface */
        auto projA = CollisionHelper::getProjectRangeOBB(obbA, axis);
        float planeProj = projA.y - manifold.m_depth * 0.5f;

        for (int i = (int) manifold.m_contacts.size() - 1; i >= 0; --i) {
            glm::vec3 &contact = manifold.m_contacts[i];

            float currentProj = glm::dot(contact, axis);
            contact += axis * (planeProj - currentProj);

            for (int j = (int) manifold.m_contacts.size() - 1; j > i; --j) {
                auto dif = manifold.m_contacts[j] - manifold.m_contacts[i];
                if (glm::dot(dif, dif) < 0.001f) {
                    manifold.m_contacts.erase(manifold.m_contacts.begin() + j);
                    break;
                }
            }
        }

        manifold.m_colliding = true;
        return manifold;
    }

    inline static OGCollisionManifold
    resolveCollision(const OBBVolume &obb, const SphereVolume &sphereVolume) {
        glm::vec3 center = obb.getCenter();

        glm::mat3 rot = glm::mat3_cast(obb.getOrientation());
        glm::vec3 localCenter = glm::transpose(rot) * (sphereVolume.getCenter() - center);
        glm::vec3 clamped = glm::clamp(localCenter, -obb.getExtents(), obb.getExtents());
        glm::vec3 closestPoint = center + rot * clamped;

        glm::vec3 diff = sphereVolume.getCenter() - closestPoint;
        float distanceSq = glm::dot(diff, diff);
        float radius = sphereVolume.getRadius();

        if (distanceSq < radius * radius) {
            OGCollisionManifold manifold;
            float distance = std::sqrt(distanceSq);
            manifold.m_colliding = true;
            manifold.m_normal = (distance > EPS) ? glm::normalize(diff) : glm::vec3(0, 1, 0);
            manifold.m_depth = radius - distance;
            manifold.m_contacts.push_back(closestPoint);
            return manifold;
        }

        return OGCollisionManifold();
    }

/*
    inline static OGCollisionManifold
    resolveCollision(const OBBVolume &obb, const AABBVolume &aabb) {
        OBBVolume obbB;
        obbB.setCenter((aabb.getMin() + aabb.getMax()) * 0.5f);
        obbB.setExtents((aabb.getMax() - aabb.getMin()) * 0.5f);
        obbB.setOrientation(glm::quat(1, 0, 0, 0));
        return resolveCollision(obb, obbB);
    }*/

    inline static OGCollisionManifold
    resolveCollision(const OBBVolume &obb, const PlaneVolume &plane) {
        OGCollisionManifold manifold;
        std::vector<glm::vec3> vertices;
        computeObbVertices(obb, vertices);

        for (const auto &v: vertices) {
            float distance = glm::dot(plane.m_normal, v) - plane.m_distance;
            if (distance <= 0.0f) {
                manifold.m_colliding = true;
                if (-distance > manifold.m_depth) {
                    manifold.m_depth = -distance;
                    manifold.m_normal = plane.m_normal;
                }
                manifold.m_contacts.push_back(v);
            }
        }

        return manifold;
    }
};

#endif //OXYOUS_2026_COLLISIONHELPER_HPP
