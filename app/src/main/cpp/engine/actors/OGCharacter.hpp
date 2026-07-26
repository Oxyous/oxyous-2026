//
// Created by Mr Steven J Baldwin on 09/07/2026.
//

#ifndef OXYOUS_2026_OGCHARACTER_HPP
#define OXYOUS_2026_OGCHARACTER_HPP

#include "OGActor.hpp"
#include "engine/animation/OGAnimController.hpp"

class OGCharacter : public OGDynamicActor {
public:

    OGCharacter() : OGDynamicActor() {
    }

    void setPath(const std::vector<glm::vec3> &path) {
        m_path = path;
        m_pathIndex = 0;
    }

    virtual void update(double deltaTime) override;

    glm::mat4 lookRotation(glm::vec3 direction, glm::vec3 up);

    virtual bool initialize() override;

protected:
    std::vector<glm::vec3> m_path;
private:
    float speed = 3.0f;
    float m_idleTime = 0.0f;
    float m_runTime = 0.0f;
    std::size_t m_pathIndex = 0;
    OGAnimController m_animationController;
    std::shared_ptr<OGAnimationClip> m_idleAnimation;
    std::shared_ptr<OGAnimationClip> m_runAnimation;
};


#endif //OXYOUS_2026_OGCHARACTER_HPP
