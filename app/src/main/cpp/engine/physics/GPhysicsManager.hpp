//
// Created by Mr Steven J Baldwin on 05/07/2026.
//

#ifndef OXYOUS_2026_GPHYSICSMANAGER_HPP
#define OXYOUS_2026_GPHYSICSMANAGER_HPP

#include <vector>
#include <memory>

class GPhysicsManager {
public:
    GPhysicsManager();
    ~GPhysicsManager();

    void update(float deltaTime);


private:
    void resolveCollisions();
    void integrate(float deltaTime);

protected:
    float m_gravity = -9.81f;

};


#endif //OXYOUS_2026_GPHYSICSMANAGER_HPP
