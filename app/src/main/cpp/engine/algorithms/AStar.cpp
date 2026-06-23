//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#include "AStar.hpp"

std::vector<std::pair<int, int>> AStar::directions = {
        {-1, 0},
        {1,  0},
        {0,  -1},
        {0,  1},
        {-1, -1},
        {-1, 1},
        {1,  -1},
        {1,  1}
};
