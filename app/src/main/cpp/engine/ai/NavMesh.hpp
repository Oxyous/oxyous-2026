//
// Created by Mr Steven J Baldwin on 06/07/2026.
//

#ifndef OXYOUS_2026_NAVMESH_HPP
#define OXYOUS_2026_NAVMESH_HPP

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>
#include "../math/MathHelper.hpp"

struct NavMeshNode {
    int polygonIndex = -1;
    glm::vec3 centroid = glm::vec3(0.0f);
    std::vector<int> neighbors;
};

class NavMesh {
public:
    NavMesh() = default;

    explicit NavMesh(const std::vector<OGPolygon> &worldPolygons, float minWalkableNormalY = 0.6f) {
        build(worldPolygons, minWalkableNormalY);
    }

    void build(const std::vector<OGPolygon> &worldPolygons, float minWalkableNormalY = 0.6f) {
        m_walkablePolygons.clear();
        m_nodes.clear();

        for (const auto &poly: worldPolygons) {
            if (std::abs(poly.normal.y) >= minWalkableNormalY) {
                m_walkablePolygons.push_back(poly);
            }
        }

        m_nodes.reserve(m_walkablePolygons.size());
        for (size_t i = 0; i < m_walkablePolygons.size(); ++i) {
            NavMeshNode node{};
            node.polygonIndex = static_cast<int>(i);
            node.centroid = computeCentroid(m_walkablePolygons[i]);
            m_nodes.push_back(node);
        }

        for (size_t i = 0; i < m_walkablePolygons.size(); ++i) {
            for (size_t j = i + 1; j < m_walkablePolygons.size(); ++j) {
                if (sharesEdgeXZ(m_walkablePolygons[i], m_walkablePolygons[j])) {
                    m_nodes[i].neighbors.push_back(static_cast<int>(j));
                    m_nodes[j].neighbors.push_back(static_cast<int>(i));
                }
            }
        }
    }

    [[nodiscard]] bool isEmpty() const { return m_nodes.empty(); }

    [[nodiscard]] const std::vector<OGPolygon> &getWalkablePolygons() const { return m_walkablePolygons; }

    [[nodiscard]] const std::vector<NavMeshNode> &getNodes() const { return m_nodes; }

    [[nodiscard]] int findContainingPolygon(const glm::vec3 &point) const {
        for (size_t i = 0; i < m_walkablePolygons.size(); ++i) {
            if (Math::pointStrictlyInsideTriangleXZ(point, m_walkablePolygons[i]) ||
                Math::pointOnPolygonBoundaryXZ(point, m_walkablePolygons[i])) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    [[nodiscard]] std::vector<int> findPolygonPath(const glm::vec3 &start, const glm::vec3 &goal) const {
        int startNode = findContainingPolygon(start);
        int goalNode = findContainingPolygon(goal);

        if (startNode < 0) {
            startNode = findNearestPolygon(start);
            if (startNode < 0) {
                aout << "NavMesh: start is outside walkable polygons" << std::endl;
                return {};
            }
            aout << "NavMesh: start snapped to nearest walkable polygon" << std::endl;
        }
        if (goalNode < 0) {
            goalNode = findNearestPolygon(goal);
            if (goalNode < 0) {
                aout << "NavMesh: goal is outside walkable polygons" << std::endl;
                return {};
            }
            aout << "NavMesh: goal snapped to nearest walkable polygon" << std::endl;
        }
        if (startNode == goalNode) {
            return {startNode};
        }

        const int n = static_cast<int>(m_nodes.size());
        std::vector<float> gScore(n, std::numeric_limits<float>::infinity());
        std::vector<float> fScore(n, std::numeric_limits<float>::infinity());
        std::vector<int> cameFrom(n, -1);
        std::vector<bool> closed(n, false);

        struct PQItem {
            float f = 0.0f;
            int node = -1;

            bool operator>(const PQItem &other) const { return f > other.f; }
        };

        std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> openSet;

        gScore[startNode] = 0.0f;
        fScore[startNode] = glm::distance(m_nodes[startNode].centroid, m_nodes[goalNode].centroid);
        openSet.push({fScore[startNode], startNode});

        while (!openSet.empty()) {
            int current = openSet.top().node;
            openSet.pop();

            if (closed[current]) {
                continue;
            }
            closed[current] = true;

            if (current == goalNode) {
                return reconstructPath(cameFrom, current);
            }

            for (int nb: m_nodes[current].neighbors) {
                if (closed[nb]) {
                    continue;
                }

                const float tentativeG = gScore[current] +
                                         glm::distance(m_nodes[current].centroid, m_nodes[nb].centroid);
                if (tentativeG < gScore[nb]) {
                    cameFrom[nb] = current;
                    gScore[nb] = tentativeG;
                    fScore[nb] = tentativeG +
                                 glm::distance(m_nodes[nb].centroid, m_nodes[goalNode].centroid);
                    openSet.push({fScore[nb], nb});
                }
            }
        }

        aout << "NavMesh: no polygon path found" << std::endl;
        return {};
    }

    [[nodiscard]] std::vector<glm::vec3> findPath(const glm::vec3 &start, const glm::vec3 &goal) const {
        const std::vector<int> polygonPath = findPolygonPath(start, goal);
        if (polygonPath.empty()) {
            return {};
        }

        std::vector<glm::vec3> waypoints;
        waypoints.reserve(polygonPath.size() + 2);
        waypoints.push_back(start);

        for (size_t i = 1; i + 1 < polygonPath.size(); ++i) {
            waypoints.push_back(m_nodes[polygonPath[i]].centroid);
        }

        waypoints.push_back(goal);
        return waypoints;
    }

private:
    int findNearestPolygon(const glm::vec3 &point) const {
        if (m_walkablePolygons.empty()) {
            return -1;
        }

        float bestDistance = std::numeric_limits<float>::infinity();
        int bestIndex = -1;
        const glm::vec2 p = {point.x, point.z};

        for (size_t i = 0; i < m_walkablePolygons.size(); ++i) {
            const auto &poly = m_walkablePolygons[i];
            const glm::vec2 a = {poly.vertices[0].x, poly.vertices[0].z};
            const glm::vec2 b = {poly.vertices[1].x, poly.vertices[1].z};
            const glm::vec2 c = {poly.vertices[2].x, poly.vertices[2].z};

            float distance = std::min({
                    Math::distToSegment2d(p, a, b),
                    Math::distToSegment2d(p, b, c),
                    Math::distToSegment2d(p, c, a)
            });

            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = static_cast<int>(i);
            }
        }

        return bestIndex;
    }

    static glm::vec3 computeCentroid(const OGPolygon &poly) {
        return (poly.vertices[0] + poly.vertices[1] + poly.vertices[2]) / 3.0f;
    }

    static bool sharesEdgeXZ(const OGPolygon &a, const OGPolygon &b, double eps = 1e-6) {
        int shared = 0;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (Math::samePointXZ(a.vertices[i], b.vertices[j], eps)) {
                    ++shared;
                }
            }
        }
        return shared >= 2;
    }

    static std::vector<int> reconstructPath(const std::vector<int> &cameFrom, int current) {
        std::vector<int> path;
        while (current != -1) {
            path.push_back(current);
            current = cameFrom[current];
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

private:
    std::vector<OGPolygon> m_walkablePolygons;
    std::vector<NavMeshNode> m_nodes;
};

#endif //OXYOUS_2026_NAVMESH_HPP
