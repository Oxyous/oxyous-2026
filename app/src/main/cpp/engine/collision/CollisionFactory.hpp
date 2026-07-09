//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#ifndef OXYOUS_2026_COLLISIONFACTORY_HPP
#define OXYOUS_2026_COLLISIONFACTORY_HPP

#include "Collision.hpp"

class CollisionFactory
{
public:
    static inline CapsuleVolume* createCapsule(glm::vec3 start, glm::vec3 end, float radius) {
        CapsuleVolume* capsule = new CapsuleVolume();
        capsule->m_base = start;
        capsule->m_top = end;
        capsule->m_radius = radius;
        return capsule;
    }

    static inline CapsuleVolume* createCapsule(float radius, float height) {
        glm::vec3 start(0.0f, 0.5f, 0.0f);
        glm::vec3 end(0.0f, height+0.5f, 0.0f);
        return createCapsule(start, end, radius);
    }

    static inline SphereVolume* createSphere(glm::vec3 center, float radius) {
        SphereVolume* sphere = new SphereVolume();
        sphere->m_center = center;
        sphere->m_radius = radius;
        return sphere;
    }

    static inline AABBVolume* createAABB(glm::vec3 min, glm::vec3 max) {
        AABBVolume* aabb = new AABBVolume();
        aabb->m_min = min;
        aabb->m_max = max;
        return aabb;
    }

    static inline OBBVolume* createOBB(glm::vec3 center, glm::vec3 extents, glm::mat3 orientation) {
        OBBVolume* obb = new OBBVolume();
        obb->m_center = center;
        obb->m_extents = extents;
        obb->m_rotation = orientation;
        return obb;
    }
};

#endif //OXYOUS_2026_COLLISIONFACTORY_HPP
