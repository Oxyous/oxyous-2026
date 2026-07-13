//
// Created by Mr Steven J Baldwin on 12/07/2026.
//

#include "Collision.hpp"
#include "CollisionFactory.hpp"
#include "CollisionHelper.hpp"

void Frustum::update(const glm::mat4& projection, const glm::mat4& view, float nearPlane, float farPlane)
{
    glm::mat4 m = projection * view;

    // Gribb-Hartmann extraction
    // Column-major: m[col][row]
    // 0: Left, 1: Right, 2: Bottom, 3: Top, 4: Near, 5: Far

    auto setup = [&](int i, float nx, float ny, float nz, float d) {
        m_planes[i].m_normal = glm::vec3(nx, ny, nz);
        m_planes[i].m_distance = d;
        float l = glm::length(m_planes[i].m_normal);
        if (l > 1e-6f) {
            m_planes[i].m_normal /= l;
            m_planes[i].m_distance /= l;
        }
    };

    setup(0, m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]); // Left
    setup(1, m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]); // Right
    setup(2, m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]); // Bottom
    setup(3, m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]); // Top
    setup(4, m[0][2],           m[1][2],           m[2][2],           m[3][2]);           // Near
    setup(5, m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]); // Far
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
        glm::vec3 p = aabb.m_min;
        if (plane.m_normal.x >= 0) p.x = aabb.m_max.x;
        if (plane.m_normal.y >= 0) p.y = aabb.m_max.y;
        if (plane.m_normal.z >= 0) p.z = aabb.m_max.z;

        if (glm::dot(plane.m_normal, p) + plane.m_distance < 0.0f) {
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
