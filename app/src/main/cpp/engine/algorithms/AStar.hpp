//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_ASTAR_HPP
#define OXYOUS_2026_ASTAR_HPP

#include "../../includes.hpp"

struct ANode {
    int x, y;
    double g, h;

    [[nodiscard]] double f() const { return g + h; }

    bool operator>(const ANode &other) const {
        return f() > other.f();
    }
};

typedef std::pair<int, int> Point;
typedef std::vector<Point> Path;
typedef std::vector<std::vector<int>> Grid;
typedef std::vector<std::vector<double>> CostMap;

class AStar {
public:
    inline static double heuristic(int x1, int y1, int x2, int y2) {
        return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }

    static std::vector<std::pair<int, int>> directions;

    inline static bool inRange(int x, int y, int width, int height) {
        return x >= 0 && x < height && y >= 0 && y < width;
    }

    inline static std::vector<Point>
    reconstructPath(std::map<Point, Point> &cameFrom, Point &current) {
        std::vector<Point> path;

        while (cameFrom.find(current) != cameFrom.end()) {
            path.push_back(current);
            current = cameFrom[current];
        }

        path.push_back(current);

        std::reverse(path.begin(), path.end());
        return path;
    }

    /* Execute A Star Algorithm */
    inline static bool execute(const Grid &grid, const Point &start, Point &end, Path &path) {
        int width = grid[0].size();
        int height = grid.size();

        std::priority_queue<ANode, std::vector<ANode>, std::greater<>> openSet;
        std::vector<std::vector<bool>> closedSet(height, std::vector<bool>(width, false));
        std::map<Point, Point> cameFrom;
        std::vector<std::vector<double>> gScore(height, std::vector<double>(width, std::numeric_limits<double>::max()));


        ANode startNode = {start.first, start.second, 0,
                         heuristic(start.first, start.second, end.first, end.second)};
        gScore[start.first][start.second] = 0;
        openSet.push(startNode);

        while (!openSet.empty()) {
            ANode current = openSet.top();
            openSet.pop();

            int x = current.x;
            int y = current.y;

            if (closedSet[x][y]) continue;
            closedSet[x][y] = true;

            if (x == end.first && y == end.second) {
                auto rePath = reconstructPath(cameFrom, end);
                for (const auto &point: rePath) {
                    path.push_back(point);
                }
                return true;
            }

            for (auto& direction : directions) {
                int nx = x + direction.first;
                int ny = y + direction.second;

                if (!AStar::inRange(nx, ny, width, height) || grid[nx][ny]) {
                    continue;
                }

                double moveCost = (direction.first == 0 || direction.second == 0) ? 1.0 : 1.41421356237; // sqrt(2.0)
                double tentativeG = gScore[x][y] + moveCost;
                if (tentativeG < gScore[nx][ny]) {
                    cameFrom[std::make_pair(nx, ny)] = std::make_pair(x, y);
                    gScore[nx][ny] = tentativeG;

                    openSet.push({nx, ny, tentativeG, heuristic(nx, ny, end.first, end.second)});
                }
            }
        }

        return false;
    }

    /* Grid to World Space */
    inline static glm::vec3 gridToWorld(const Point& point, float scale, float height = 0.0f) {
        return {point.first * scale, height + 2.0, point.second * scale};
    }

    /* World to Grid Space */
    inline static Point worldToGrid(const glm::vec3& point, float scale) {
        return { point.x / scale, point.z / scale};
    }


};


#endif //OXYOUS_2026_ASTAR_HPP
