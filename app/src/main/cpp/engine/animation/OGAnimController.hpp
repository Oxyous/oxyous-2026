//
// Created by Mr Steven J Baldwin on 21/07/2026.
//

#ifndef OXYOUS_2026_OGANIMCONTROLLER_HPP
#define OXYOUS_2026_OGANIMCONTROLLER_HPP

#include "../../DataStructures.hpp"
#include <memory>

class OGAnimController {
public:
    OGAnimController() = default;

    ~OGAnimController() = default;

public:
    /** Play Animation */
    void playAnimation(std::shared_ptr<OGAnimationClip> clip);

    /** Update Animation Controller*/
    void update(float deltaTime);

    /** Get Current Animation */
    std::shared_ptr<OGAnimationClip> getCurrentAnimation() const;

    /** Get Previous Animation */
    std::shared_ptr<OGAnimationClip> getPreviousAnimation() const;

    /** Get Current Pose */
    OGAnimationPose getCurrentPose() const;

    /** Get Current Global Matrices */
    std::vector<glm::mat4> getCurrentGlobalMatrices(const std::vector<OGJointInfo>& hierarchy) const;

private:
    /** Sample Animation */
    OGAnimationPose SampleAnimation(std::shared_ptr<OGAnimationClip> clip, float time) const;

    /** Blend Pose */
    OGAnimationPose BlendPose(OGAnimationPose previousPose, OGAnimationPose currentPose, float blendFactor) const;

private:
    std::shared_ptr<OGAnimationClip> m_current = nullptr;
    std::shared_ptr<OGAnimationClip> m_previous = nullptr;
    float m_currentTime = 0.0f;
    float m_blendTime = 0.0f;
    float m_blendDuration = 0.2f;
};


#endif //OXYOUS_2026_OGANIMCONTROLLER_HPP
