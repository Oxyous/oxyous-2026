//
// Created by Mr Steven J Baldwin on 26/06/2026.
//

#include "BHV.hpp"

/**/
void BHV::build(const ::std::vector<OGPolygon> &inPolygons, int maxLeafSize) {
    this->polygons = inPolygons;
    this->indices.resize(inPolygons.size());
    std::iota(indices.begin(), indices.end(), 0);
    this->nodes.clear();
    if (!inPolygons.empty()) {
        buildRecursive(0, inPolygons.size(), maxLeafSize);
    } else {
        root = -1;
    }
}

int BHV::buildRecursive(int start, int end, int maxLeafSize) {
    BHVNode node;
    AABBVolume box;
    AABBVolume centroidBox;

    for (int i = start; i < end; i++) {
        const OGPolygon &poly = polygons[indices[i]];
        box.addVolume(CollisionHelper::PolygonBounds(poly));
        centroidBox.addPoint(CollisionHelper::PolygonCentroid(poly));
    }

    node.aabb = box;
    int count = end - start;

    int nodeIndex = static_cast<int>(nodes.size());
    nodes.push_back(node);

    if (count <= maxLeafSize) {
        node.leaf = true;
        node.start = start;
        node.count = count;
        return nodeIndex;
    }

    glm::vec3 extent = centroidBox.getMax() - centroidBox.getMin();
    int axis = 0;

    if (extent.y > extent.x && extent.y > extent.z) axis = 1;
    else if (extent.z > extent.x && extent.z > extent.y) axis = 2;

    int mid = (start + end) / 2;
    std::nth_element(indices.begin() + start, indices.begin() + mid, indices.begin() + end,
                     [&](int lhs, int rhs) {
                         glm::vec3 centroidL = CollisionHelper::PolygonCentroid(polygons[lhs]);
                         glm::vec3 centroidR = CollisionHelper::PolygonCentroid(polygons[rhs]);

                         if (axis == 0) return centroidL.x < centroidR.x;
                         if (axis == 1) return centroidL.y < centroidR.y;
                         return centroidL.z < centroidR.z;
                     });

    int leftChild = buildRecursive(start, mid, maxLeafSize);
    int rightChild = buildRecursive(mid, end, maxLeafSize);

    nodes[nodeIndex].left = leftChild;
    nodes[nodeIndex].right = rightChild;
    nodes[nodeIndex].leaf = false;

    return nodeIndex;
}

/* Segment Intersects Polygon Volume*/
bool BHV::segmentIntersectsPolygon(int nodeIndex, const glm::vec3 &a, const glm::vec3 &b,
                                   double ignoreMin, double ignoreMax) {
    const BHVNode &node = nodes[nodeIndex];

    if (!CollisionHelper::segmentIntersectsAabb(a, b, node.aabb)) {
        return false;
    }

    if (node.leaf) {
        for (int i = 0; i < node.count; i++) {
            const OGPolygon &poly = polygons[indices[node.start + i]];
            double t;
            if (CollisionHelper::segmentIntersectsPolygon(a, b, poly, t)) {
                if (t >= ignoreMin && t <= ignoreMax) {
                    return true;
                }
            }
        }
        return false;
    }

    if (segmentIntersectsPolygon(node.left, a, b, ignoreMin, ignoreMax)) {
        return true;
    }

    if (segmentIntersectsPolygon(node.right, a, b, ignoreMin, ignoreMax)) {
        return true;
    }

    return false;
}