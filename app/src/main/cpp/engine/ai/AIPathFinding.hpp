//
// Created by Mr Steven J Baldwin on 06/07/2026.
//

#ifndef OXYOUS_2026_AIPATHFINDING_HPP
#define OXYOUS_2026_AIPATHFINDING_HPP

#include <vector>
#include <limits>
#include "../math/MathHelper.hpp"

/** Path Node For AI */
struct PathNode {
    glm::vec3 position;
    std::vector<int> neighbors;
};

struct AIPathGraph {
    std::vector<PathNode> nodes;
};

/** */
class AIPathFinding {
public:
    AIPathFinding();

    virtual ~AIPathFinding();

    /**  */
    static AIPathGraph buildVisibilityGraphFromPolygons(
            const std::vector<OGPolygon> &polygons,
            const glm::vec3 &start,
            const glm::vec3 &goal,
            double maxConnectionsDistance = std::numeric_limits<double>::infinity()) {
        for (const auto &poly: polygons) {
            if (Math::pointStrictlyInsideTriangleXZ(start, poly)) {
                throw std::runtime_error("Start lies within a triangle obstacle");
            }
            if (Math::pointStrictlyInsideTriangleXZ(goal, poly)) {
                throw std::runtime_error("Goal lies within a triangle obstacle");
            }
        }

        std::vector<glm::vec3> points;
        points.reserve(2 + polygons.size() * 3);

        Math::addUniquePoint(points, start);
        Math::addUniquePoint(points, goal);

        for (const auto &poly: polygons) {
            Math::addUniquePoint(points, poly.vertices[0]);
            Math::addUniquePoint(points, poly.vertices[1]);
            Math::addUniquePoint(points, poly.vertices[2]);
        }

        AIPathGraph graph;
        graph.nodes.reserve(points.size());
        for (const auto &p: points) {
            graph.nodes.push_back({p, {}});
        }

        for (size_t i = 0; i < points.size(); ++i) {
            for (size_t j = i + 1; j < points.size(); ++j) {
                double dist = glm::distance(points[i], points[j]);
                if (dist <= maxConnectionsDistance) {
                    if (Math::segmentIsClearXZ(points[i], points[j], polygons)) {
                        graph.nodes[i].neighbors.push_back(static_cast<int>(j));
                        graph.nodes[j].neighbors.push_back(static_cast<int>(i));
                    }
                }
            }
        }

        return graph;
    }

    /** Reconstruct Path */
    static std::vector<int> reconstructPath(std::vector<int> &cameFrom, int current) {
        std::vector<int> path;
        while (current != -1) {
            path.push_back(current);
            current = cameFrom[current];
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    /** Perform A Start Path finding Algorithm on Graph Node*/
    static std::vector<int> aStar(const AIPathGraph &graph, int startId, int goalId) {
        const int n = static_cast<int>(graph.nodes.size());
        std::vector<double> gScore(n, std::numeric_limits<double>::infinity());
        std::vector<double> fScore(n, std::numeric_limits<double>::infinity());
        std::vector<int> cameFrom(n, -1);
        std::vector<bool> closed(n, false);

        struct PQItem {
            double f;
            int node;

            bool operator>(const PQItem &other) const { return f > other.f; }
        };

        std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> openSet;
        gScore[startId] = 0.0;
        fScore[startId] = glm::distance(graph.nodes[startId].position,
                                        graph.nodes[goalId].position);
        openSet.push({fScore[startId], startId});

        while (!openSet.empty()) {
            int current = openSet.top().node;
            openSet.pop();

            if (closed[current]) continue;
            closed[current] = true;

            if (current == goalId) {
                return reconstructPath(cameFrom, current);
            }

            for (int nb: graph.nodes[current].neighbors) {
                if (closed[nb]) continue;

                double tentativeGScore = gScore[current] + glm::distance(graph.nodes[current].position,
                                                                         graph.nodes[nb].position);
                if (tentativeGScore < gScore[nb]) {
                    cameFrom[nb] = current;
                    gScore[nb] = tentativeGScore;
                    fScore[nb] = gScore[nb] + glm::distance(graph.nodes[nb].position,
                                                            graph.nodes[goalId].position);
                    openSet.push({fScore[nb], nb});
                }
            }
        }

        /** No Path Found */
        return {};
    }

    /** Smooth Path */
    static std::vector<int> smoothPath(const AIPathGraph& graph, const std::vector<int>& path,
                                       const std::vector<OGPolygon>& polygons) {
        if (path.size() < 2) return path;

        std::vector<int> results;
        size_t i = 0;

        results.push_back(path[i]);
        while (i < path.size()) {
            size_t furthest = i + 1;
            for(size_t j = path.size()-1; j > i; --j) {
                if (Math::segmentIsClearXZ(graph.nodes[path[i]].position, graph.nodes[path[j]].position, polygons)) {
                    furthest = j;
                    break;
                }
            }
            results.push_back(path[furthest]);
            i = furthest;
        }

        return results;
    }

    /** Find Path */
    static std::vector<glm::vec3> findPath(const std::vector<OGPolygon>& polygons,
                                    const glm::vec3& start,
                                    const glm::vec3& goal,
                                    double maxConnectionsDistance = std::numeric_limits<double>::infinity()) {
        AIPathGraph graph = buildVisibilityGraphFromPolygons(polygons, start, goal, maxConnectionsDistance);
        int startId = 0; // Start is the first point added
        int goalId = 1;  // Goal is the second point added

        std::vector<int> pathIndices = aStar(graph, startId, goalId);
        if (pathIndices.empty()) {
            throw std::runtime_error("No path found between start and goal.");
        }

        std::vector<int> smoothedPathIndices = smoothPath(graph, pathIndices, polygons);

        std::vector<glm::vec3> path;
        for (int index : smoothedPathIndices) {
            path.push_back(graph.nodes[index].position);
        }

        return path;
    }
};


#endif //OXYOUS_2026_AIPATHFINDING_HPP
