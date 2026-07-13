//
// Created by Mr Steven J Baldwin on 12/07/2026.
//

#ifndef OXYOUS_2026_OGBVH_HPP
#define OXYOUS_2026_OGBVH_HPP

#include <vector>
#include <numeric>
#include "engine/collision/Collision.hpp"

struct OGBVHNode {
    AABBVolume aabb;
    int left = -1;
    int right = -1;
    int start = 0;
    int count = 0;
    bool leaf = false;
};

template<class T>
class OGBVH {
public:
    OGBVH() {}

    void build(std::vector<T> &primitives, int maxLeafSize = 4) {
        m_primitives = primitives;
        nodes.clear();
        indices.resize(m_primitives.size());
        std::iota(indices.begin(), indices.end(), 0);

        if (m_primitives.empty()) {
            root = -1;
            return;
        }

        root = buildRecursive(0, 0, static_cast<int>(indices.size()), maxLeafSize);
    }

    void intersectsFrustum(const Frustum &frustum, std::vector<T> &results) {
        if (root != -1) {
            intersectsRecursive(root, frustum, results);
        }
    }

    void intersects(const AABBVolume &volume, std::vector<T> &results) {
        if (root != -1) {
            intersectsRecursive(root, volume, results);
        }

    }

    int getRootIndex() {
        return root;
    }

private:

    void intersectsRecursive(int nodeIndex, const Frustum &frustum, std::vector<T> &results) {
        const auto &node = nodes[nodeIndex];

        if (!frustum.intersects(node.aabb)) {
            return;
        }

        if (node.leaf) {
            for (int i = 0; i < node.count; ++i) {
                results.push_back(m_primitives[indices[node.start + i]]);
            }
        } else {
            intersectsRecursive(node.left, frustum, results);
            intersectsRecursive(node.right, frustum, results);
        }
    }

    void intersectsRecursive(int nodeIndex, const AABBVolume& aabb, std::vector<T> &results){
        const auto &node = nodes[nodeIndex];

        if (aabb.intersect(node.aabb) == false) {
            return;
        }

        if (node.leaf) {
            for (int i = 0; i < node.count; ++i) {
                results.push_back(m_primitives[indices[node.start + i]]);
            }
        } else {
            intersectsRecursive(node.left, aabb, results);
            intersectsRecursive(node.right, aabb, results);
        }
    }

    int buildRecursive(int depth, int start, int end, int maxLeafSize) {
        int nodeIdx = static_cast<int>(nodes.size());
        nodes.emplace_back();

        OGBVHNode node;
        node.start = start;
        node.count = end - start;
        computeAABB(start, end, node.aabb);

        if (node.count <= maxLeafSize || node.count <= 1) {
            node.leaf = true;
            nodes[nodeIdx] = node;
            return nodeIdx;
        }

        int axis = findSplitAxis(start, end);
        float splitPos = node.aabb.getCentroid()[axis];

        int mid = start;
        for (int i = start; i < end; i++) {
            if (m_primitives[indices[i]].getCentroid()[axis] < splitPos) {
                std::swap(indices[i], indices[mid]);
                mid++;
            }
        }

        if (mid == start || mid == end) {
            mid = start + (end - start) / 2;
        }

        nodes[nodeIdx] = node;
        nodes[nodeIdx].left = buildRecursive(depth + 1, start, mid, maxLeafSize);
        nodes[nodeIdx].right = buildRecursive(depth + 1, mid, end, maxLeafSize);

        return nodeIdx;
    }

    int findSplitAxis(int start, int end) {
        AABBVolume aabb;
        computeAABB(start, end, aabb);

        glm::vec3 extents = aabb.m_max - aabb.m_min;
        if (extents.x >= extents.y && extents.x >= extents.z) {
            return 0; // X-axis
        } else if (extents.y >= extents.x && extents.y >= extents.z) {
            return 1; // Y-axis
        } else {
            return 2; // Z-axis
        }
    }

    void computeAABB(int start, int end, AABBVolume &aabb) {
        if (start >= end) {
            aabb.m_min = glm::vec3(0.0f);
            aabb.m_max = glm::vec3(0.0f);
            return;
        }

        aabb = m_primitives[indices[start]];
        for (int i = start + 1; i < end; i++) {
            aabb.addVolume(m_primitives[indices[i]]);
        }
    }

private:
    std::vector<OGBVHNode> nodes;
    std::vector<T> m_primitives;
    std::vector<int> indices;
    int root = -1;
};


#endif //OXYOUS_2026_OGBVH_HPP
