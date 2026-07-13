//
// Created by Mr Steven J Baldwin on 13/07/2026.
//

#ifndef OXYOUS_2026_OGOCTREE_HPP
#define OXYOUS_2026_OGOCTREE_HPP

#include "../../includes.hpp"
#include "DataStructures.hpp"
#include "engine/collision/Collision.hpp"
#include "engine/collision/CollisionHelper.hpp"
#include "../algorithms/OGBVH.hpp"

template<typename T>
struct OctreeNode {
    struct Entry {
        T item;
        AABBVolume aabb;
    };

    glm::vec3 center;
    float halfDimension;
    std::vector<Entry> data;
    std::unique_ptr<OctreeNode<T>> children[8];
    std::unique_ptr<OGBVH<T>> bvh;

    OctreeNode(glm::vec3 c, float h) : center(c), halfDimension(h) {
        for (int i = 0; i < 8; ++i) children[i] = nullptr;
    }

    bool isLeaf() const {
        for (int i = 0; i < 8; ++i) if (children[i]) return false;
        return true;
    }

    void buildBVH() {
        if (!data.empty()) {
            bvh = std::make_unique<OGBVH<T>>();
            std::vector<T> items;
            items.reserve(data.size());
            for (const auto &entry: data) items.push_back(entry.item);
            bvh->build(items);
        }
        for (int i = 0; i < 8; ++i) {
            if (children[i]) children[i]->buildBVH();
        }
    }

    void insert(T item, const AABBVolume &itemAABB, int depth = 0) {
        if (depth >= 8) {
            data.push_back({item, itemAABB});
            return;
        }

        // Subdivide if this is a leaf and we have enough data
        if (isLeaf() && data.size() >= 16) {
            float newHalf = halfDimension * 0.5f;
            for (int i = 0; i < 8; ++i) {
                glm::vec3 newCenter = center;
                newCenter.x += (i & 1) ? newHalf : -newHalf;
                newCenter.y += (i & 2) ? newHalf : -newHalf;
                newCenter.z += (i & 4) ? newHalf : -newHalf;
                children[i] = std::make_unique<OctreeNode<T>>(newCenter, newHalf);
            }

            std::vector<Entry> oldData = std::move(data);
            data.clear();
            for (const auto &entry: oldData) {
                insert(entry.item, entry.aabb, depth); // Recursive call will now use children
            }
        }

        float newHalf = halfDimension * 0.5f;
        int index = -1;

        if (!isLeaf()) {
            // Use a small epsilon to avoid straddling when just touching a boundary
            const float epsilon = 0.001f;
            bool completelyOnLeft = itemAABB.getMax().x < center.x - epsilon;
            bool completelyOnRight = itemAABB.getMin().x > center.x + epsilon;
            bool straddlesX = !completelyOnLeft && !completelyOnRight;

            bool completelyOnBottom = itemAABB.getMax().y < center.y - epsilon;
            bool completelyOnTop = itemAABB.getMin().y > center.y + epsilon;
            bool straddlesY = !completelyOnBottom && !completelyOnTop;

            bool completelyOnBack = itemAABB.getMax().z < center.z - epsilon;
            bool completelyOnFront = itemAABB.getMin().z > center.z + epsilon;
            bool straddlesZ = !completelyOnBack && !completelyOnFront;

            if (!straddlesX && !straddlesY && !straddlesZ) {
                index = 0;
                if (completelyOnRight) index |= 1;
                if (completelyOnTop) index |= 2;
                if (completelyOnFront) index |= 4;
            }
        }

        if (index != -1) {
            children[index]->insert(item, itemAABB, depth + 1);
        } else {
            data.push_back({item, itemAABB});
        }
    }

    void query(const AABBVolume &queryBox, std::vector<T> &results) const {
        glm::vec3 nodeMin = center - glm::vec3(halfDimension);
        glm::vec3 nodeMax = center + glm::vec3(halfDimension);
        if (!queryBox.intersect(AABBVolume(nodeMin, nodeMax))) return;

        if (bvh) {
            bvh->intersects(queryBox, results);
        } else {
            for (const auto &entry: data) {
                if (queryBox.intersect(entry.aabb)) {
                    results.push_back(entry.item);
                }
            }
        }

        for (int i = 0; i < 8; ++i) {
            if (children[i]) children[i]->query(queryBox, results);
        }
    }

    void query(const Frustum &frustum, std::vector<T> &results) const {
        glm::vec3 nodeMin = center - glm::vec3(halfDimension);
        glm::vec3 nodeMax = center + glm::vec3(halfDimension);
        AABBVolume nodeAABB(nodeMin, nodeMax);

        if (!frustum.intersects(nodeAABB)) return;

        if (bvh) {
            bvh->intersectsFrustum(frustum, results);
        } else {
            for (const auto &entry: data) {
                if (frustum.intersects(entry.aabb)) {
                    results.push_back(entry.item);
                }
            }
        }

        for (int i = 0; i < 8; ++i) {
            if (children[i]) children[i]->query(frustum, results);
        }
    }

    void query(glm::vec3 min, glm::vec3 max, std::vector<T> &results) const {
        query(AABBVolume(min, max), results);
    }
};

template<typename T>
class OGOctree {
public:
    OGOctree() : root(nullptr) {}

    OGOctree(glm::vec3 center, float halfDimension)
            : root(std::make_unique<OctreeNode<T>>(center, halfDimension)) {}

    void insert(T item, const AABBVolume &aabb) {
        if (root) root->insert(item, aabb);
    }

    void query(const AABBVolume &queryBox, std::vector<T> &results) const {
        results.clear();
        if (root) root->query(queryBox, results);
    }

    void query(const Frustum &frustum, std::vector<T> &results) const {
        results.clear();
        if (root) root->query(frustum, results);
    }

    void build(const std::vector<OGPolygon> &polygons) {
        if (polygons.empty()) return;
        AABBVolume rootBound;
        for (const auto &poly: polygons) {
            rootBound.addVolume(poly);
        }

        glm::vec3 center = (rootBound.m_min + rootBound.m_max) * 0.5f;
        glm::vec3 size = rootBound.m_max - rootBound.m_min;
        float halfDim = std::max({size.x, size.y, size.z}) * 0.5f;

        root = std::make_unique<OctreeNode<T>>(center, halfDim);

        for (const auto &poly: polygons) {
            root->insert(poly, CollisionHelper::PolygonBounds(poly));
        }

        root->buildBVH();
    }

    void build(const std::vector<AABBVolume> &volumes) {
        if (volumes.empty()) return;
        AABBVolume rootBound;
        for (const auto &vol: volumes) {
            rootBound.addVolume(vol);
        }

        glm::vec3 center = (rootBound.m_min + rootBound.m_max) * 0.5f;
        glm::vec3 size = rootBound.m_max - rootBound.m_min;
        float halfDim = std::max({size.x, size.y, size.z}) * 0.5f;

        root = std::make_unique<OctreeNode<T>>(center, halfDim);

        for (const auto &vol: volumes) {
            root->insert(vol, vol);
        }

        root->buildBVH();
    }

private:
    std::unique_ptr<OctreeNode<T>> root;
};


#endif //OXYOUS_2026_OGOCTREE_HPP
