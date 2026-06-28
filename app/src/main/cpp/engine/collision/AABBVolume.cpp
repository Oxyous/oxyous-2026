//
// Created by Mr Steven J Baldwin on 28/06/2026.
//

#include "Collision.hpp"

bool AABBVolume::intersect(const Ray& ray, RaycastHit& hit) const
{
    return true;
}

bool AABBVolume::intersect(const SphereVolume& sphereVolume) const {
    return true;
}


/* Intersect AABB with Volume */
bool AABBVolume::intersect(const AABBVolume& aabb) const
{
    return true;
}

/* Intersect OBB with Volume */
bool AABBVolume::intersect(const OBBVolume& obb) const {
    return true;
}

