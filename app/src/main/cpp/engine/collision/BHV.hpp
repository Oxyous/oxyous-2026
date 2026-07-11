//
// Created by Mr Steven J Baldwin on 26/06/2026.
//

#ifndef OXYOUS_2026_BHV_HPP
#define OXYOUS_2026_BHV_HPP

#include <numeric>
#include "Collision.hpp"
#include "CollisionHelper.hpp"

typedef struct BHVNode {
    AABBVolume aabb;
    int left = -1;
    int right = -1;
    int start = 0;
    int count = 0;
    bool leaf = false;
} BHVNode;

class BHV {
public:
    std::vector<OGPolygon> polygons;
    std::vector<int> indices;
    std::vector<BHVNode> nodes;
    int root = -1;

    void build(const ::std::vector<OGPolygon> &inPolygons, int maxLeafSize = 4);

    void intersects(const OBBVolume& obb, std::vector<OGPolygon>& results);

    void intersects(const CapsuleVolume& capsule, std::vector<OGPolygon>& results);

    void intersects(const SphereVolume& sphere, std::vector<OGPolygon>& results);

private:
    int buildRecursive(int start, int end, int maxLeafSize);

    void intersectsRecursive(int nodeIndex, const OBBVolume& obb, std::vector<OGPolygon>& results);

    void intersectsRecursive(int nodeIndex, const CapsuleVolume& capsule, std::vector<OGPolygon>& results);

    bool segmentIntersectsPolygon(int nodeIndex, const glm::vec3 &a, const glm::vec3 &b, double ignoreMin, double ignoreMax, OGPolygon& polyOut);

    void intersectsRecursive(int nodeIndex, const SphereVolume& sphere, std::vector<OGPolygon>& results);
};


#endif //OXYOUS_2026_BHV_HPP
