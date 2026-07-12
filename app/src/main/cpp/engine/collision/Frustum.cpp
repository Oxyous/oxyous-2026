//
// Created by Mr Steven J Baldwin on 12/07/2026.
//

#include "Collision.hpp"
#include "CollisionFactory.hpp"
#include "CollisionHelper.hpp"

void Frustum::update(const glm::mat4& projection, const glm::mat4& view, float nearPlane, float farPlane)
{
    auto corners = CollisionHelper::getFrustumCornersWorldSpace(projection, view, nearPlane, farPlane);

    // Near, Far, Left, Right, Top, Bottom
    // Corners: 0-3 are near, 4-7 are far. Actual Order from CollisionHelper: BL, BR, TL, TR
    m_planes[0] = CollisionFactory::computePlaneFromPoints(corners[0], corners[2], corners[1]); // Near (BL, TL, BR -> normal -Z)
    m_planes[1] = CollisionFactory::computePlaneFromPoints(corners[4], corners[5], corners[6]); // Far (BLF, BRF, TLF -> normal +Z)
    m_planes[2] = CollisionFactory::computePlaneFromPoints(corners[0], corners[4], corners[6]); // Left (BL, BLF, TLF -> normal +X)
    m_planes[3] = CollisionFactory::computePlaneFromPoints(corners[1], corners[3], corners[7]); // Right (BR, TR, TRF -> normal -X)
    m_planes[4] = CollisionFactory::computePlaneFromPoints(corners[2], corners[6], corners[7]); // Top (TL, TLF, TRF -> normal -Y)
    m_planes[5] = CollisionFactory::computePlaneFromPoints(corners[0], corners[1], corners[5]); // Bottom (BL, BR, BRF -> normal +Y)
}

bool Frustum::intersects(const SphereVolume& sphere) const
{
    for (const auto& plane : m_planes) {
        if (glm::dot(glm::vec3(plane.m_normal), sphere.getCenter()) + plane.m_distance < -sphere.getRadius()) {
            return false;
        }
    }
    return true;
}

bool Frustum::intersects(const AABBVolume &aabb) const {
    for (const auto& plane : m_planes) {
        glm::vec3 p = aabb.getMin();
        if (plane.m_normal.x >= 0) p.x = aabb.getMax().x;
        if (plane.m_normal.y >= 0) p.y = aabb.getMax().y;
        if (plane.m_normal.z >= 0) p.z = aabb.getMax().z;

        if (glm::dot(plane.m_normal, p) + plane.m_distance < 0) {
            return false;
        }
    }
    return true;
}

bool Frustum::intersects(const OBBVolume &obb) const {
    for (const auto& plane : m_planes) {
        // Project the OBB half-extents onto the plane normal
        float r = obb.getExtents().x * std::abs(glm::dot(plane.m_normal, glm::mat3_cast(obb.getOrientation())[0])) +
                  obb.getExtents().y * std::abs(glm::dot(plane.m_normal, glm::mat3_cast(obb.getOrientation())[1])) +
                  obb.getExtents().z * std::abs(glm::dot(plane.m_normal, glm::mat3_cast(obb.getOrientation())[2]));

        float distance = glm::dot(plane.m_normal, obb.getCenter()) + plane.m_distance;

        if (distance < -r) {
            return false;
        }
    }
    return true;
}

bool Frustum::intersects(const glm::vec3& point) const {
    for (const auto& plane : m_planes) {
        if (glm::dot(plane.m_normal, point) + plane.m_distance < 0) {
            return false;
        }
    }
    return true;
}
