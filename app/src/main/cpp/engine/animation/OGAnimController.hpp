//
// Created by Mr Steven J Baldwin on 21/07/2026.
//

#ifndef OXYOUS_2026_OGANIMCONTROLLER_HPP
#define OXYOUS_2026_OGANIMCONTROLLER_HPP

#include "../../DataStructures.hpp"

class OGAnimController {
public:
    OGAnimController() = default;

    ~OGAnimController() = default;

public:
    /** Play Animation */
    void playAnimation(OGAnimationClip *clip);

    /** Update Animation Controller*/
    void update(float deltaTime);

    /** Get Current Animation */
    OGAnimationClip *getCurrentAnimation() const;

    /** Get Previous Animation */
    OGAnimationClip *getPreviousAnimation() const;

    /** Get Current Pose */
    OGAnimationPose getCurrentPose() const;

private:
    /** Sample Animation */
    OGAnimationPose SampleAnimation(OGAnimationClip *clip, float time) const;

    /** Blend Pose */
    OGAnimationPose BlendPose(OGAnimationPose previousPose, OGAnimationPose currentPose, float blendFactor) const;

private:
    OGAnimationClip *m_current = nullptr;
    OGAnimationClip *m_previous = nullptr;
    float m_currentTime = 0.0f;
    float m_blendTime = 0.0f;
    float m_blendDuration = 0.2f;
};


#endif //OXYOUS_2026_OGANIMCONTROLLER_HPP
