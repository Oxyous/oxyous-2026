//
// Created by Mr Steven J Baldwin on 21/07/2026.
//

#include "OGAnimController.hpp"

OGAnimationPose OGAnimController::SampleAnimation(OGAnimationClip *clip, float time) const {
    OGAnimationPose result;

    if (!clip) return result;

    float frameTime = 1.0f / clip->frameRate;
    float frameFloat = time / frameTime;

    int frameA = static_cast<int>(floor(frameFloat));
    int frameB = (frameA + 1) % clip->frames.size();

    float t = frameFloat - frameA;

    frameA %= clip->frames.size();

    OGAnimationPose &poseA = clip->frames[frameA];
    OGAnimationPose &poseB = clip->frames[frameB];

    result.joints.resize(poseA.joints.size());

    for (size_t i = 0; i < poseA.joints.size(); ++i) {
        result.joints[i].position = glm::mix(poseA.joints[i].position, poseB.joints[i].position, t);
        result.joints[i].orientation = glm::slerp(poseA.joints[i].orientation,
                                                  poseB.joints[i].orientation, t);
    }
    return result;
}

OGAnimationPose
OGAnimController::BlendPose(OGAnimationPose previousPose, OGAnimationPose currentPose,
                            float blendFactor) const {

    OGAnimationPose result;
    result.joints.resize(currentPose.joints.size());

    for (size_t i = 0; i < currentPose.joints.size(); ++i) {
        result.joints[i].position = glm::mix(previousPose.joints[i].position,
                                             currentPose.joints[i].position,
                                             blendFactor);
        result.joints[i].orientation = glm::slerp(previousPose.joints[i].orientation,
                                                  currentPose.joints[i].orientation, blendFactor);
    }
    return result;
}

OGAnimationPose OGAnimController::getCurrentPose() const {
    OGAnimationPose currentPose = SampleAnimation(m_current, m_currentTime);

    if (!m_previous || m_blendTime >= m_blendDuration) {
        return currentPose;
    }

    float blendFactor = m_blendTime / m_blendDuration;

    OGAnimationPose previousPose = SampleAnimation(m_previous, m_currentTime);

    return BlendPose(previousPose, currentPose, blendFactor);
}

void OGAnimController::update(float deltaTime) {
    if (!m_current)
        return;

    m_currentTime += deltaTime;

    while (m_currentTime > m_current->duration)
        m_currentTime -= m_current->duration;

    if (m_blendTime < m_blendDuration)
        m_blendTime += deltaTime;
}

OGAnimationClip *OGAnimController::getCurrentAnimation() const {
    return m_current;
}

OGAnimationClip *OGAnimController::getPreviousAnimation() const {
    return m_previous;
}

void OGAnimController::playAnimation(OGAnimationClip *clip) {

    if (m_current == clip)
        return;

    m_previous = m_current;
    m_current = clip;

    m_currentTime = 0.0f;
    m_blendTime = 0.0f;
    m_blendDuration = 0.2f;
}
