//
// Created by Mr Steven J Baldwin on 21/07/2026.
//

#include "OGAnimController.hpp"

OGAnimationPose OGAnimController::SampleAnimation(std::shared_ptr<OGAnimationClip> clip, float time) const {
    OGAnimationPose result;

    if (!clip || clip->skeletonFrames.empty()) return result;

    if (clip->frameRate <= 0.0f || clip->skeletonFrames.empty()) {
        // Fallback to something sane
        size_t numJoints = clip->joints.size();
        result.joints.resize(numJoints);
        if (!clip->skeletonFrames.empty()) {
            auto &pose = clip->skeletonFrames[0];
            for(size_t i=0; i<numJoints && i<pose.joints.size(); ++i) {
                result.joints[i].position = pose.joints[i].position;
                result.joints[i].orientation = pose.joints[i].orientation;
            }
        } else {
            for(size_t i=0; i<numJoints; ++i) {
                result.joints[i].position = glm::vec3(0.0f);
                result.joints[i].orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            }
        }
        return result;
    }

    float frameTime = 1.0f / clip->frameRate;
    float frameFloat = time / frameTime;

    int frameA = static_cast<int>(floor(frameFloat));
    float t = frameFloat - static_cast<float>(frameA);

    size_t numFrames = clip->skeletonFrames.size();
    frameA = (frameA % (int)numFrames + (int)numFrames) % (int)numFrames;
    int frameB = (frameA + 1) % numFrames;

    if (frameA >= numFrames || frameB >= numFrames) return result;

    auto &poseA = clip->skeletonFrames[frameA];
    auto &poseB = clip->skeletonFrames[frameB];

    size_t numJoints = poseA.joints.size();
    if (numJoints > 1000) {
        aout << "Error: numJoints is suspiciously large: " << numJoints << " for clip " << (clip->name.empty() ? "unnamed" : clip->name) << std::endl;
        return result;
    }

    static bool loggedOnce = false;
    if (!loggedOnce) {
        aout << "Animation Sample: " << (clip->name.empty() ? "unnamed" : clip->name) << " has " << numJoints << " joints" << std::endl;
        for (size_t i = 0; i < std::min(numJoints, (size_t)5); ++i) {
            aout << "Joint " << i << " pos: " << poseA.joints[i].position.x << ", " << poseA.joints[i].position.y << ", " << poseA.joints[i].position.z << " parent: " << (i < clip->joints.size() ? clip->joints[i].parentIndex : -2) << std::endl;
        }
        loggedOnce = true;
    }

    result.joints.resize(numJoints);

    for (size_t i = 0; i < numJoints; ++i) {
        if (i >= poseB.joints.size()) {
            result.joints[i].position = poseA.joints[i].position;
            result.joints[i].orientation = poseA.joints[i].orientation;
            continue;
        }

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
    size_t numJoints = currentPose.joints.size();
    result.joints.resize(numJoints);

    for (size_t i = 0; i < numJoints; ++i) {
        if (i >= previousPose.joints.size()) {
            result.joints[i] = currentPose.joints[i];
            continue;
        }

        result.joints[i].position = glm::mix(previousPose.joints[i].position,
                                             currentPose.joints[i].position,
                                             blendFactor);
        result.joints[i].orientation = glm::slerp(previousPose.joints[i].orientation,
                                                  currentPose.joints[i].orientation, blendFactor);
    }
    return result;
}

OGAnimationPose OGAnimController::getCurrentPose() const {
    if (!m_current) return OGAnimationPose();

    OGAnimationPose currentPose = SampleAnimation(m_current, m_currentTime);

    if (!m_previous || m_blendTime >= m_blendDuration) {
        return currentPose;
    }

    float blendFactor = m_blendTime / m_blendDuration;

    OGAnimationPose previousPose = SampleAnimation(m_previous, m_currentTime);

    return BlendPose(previousPose, currentPose, blendFactor);
}

std::vector<glm::mat4> OGAnimController::getCurrentGlobalMatrices(const std::vector<OGJointInfo> &hierarchy) const {
    OGAnimationPose pose = getCurrentPose();
    size_t numJoints = pose.joints.size();
    if (numJoints == 0) return std::vector<glm::mat4>();

    std::vector<glm::mat4> globalMatrices(numJoints, glm::mat4(1.0f));
    std::vector<bool> resolved(numJoints, false);

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < numJoints; ++i) {
            if (resolved[i]) continue;
            if (i >= hierarchy.size()) {
                resolved[i] = true;
                changed = true;
                continue;
            }

            int parentIdx = hierarchy[i].parentIndex;
            if (parentIdx < 0 || (parentIdx < (int)numJoints && resolved[parentIdx])) {
                glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), pose.joints[i].position) *
                                        glm::mat4_cast(pose.joints[i].orientation);
                if (parentIdx >= 0) {
                    globalMatrices[i] = globalMatrices[parentIdx] * localMatrix;
                } else {
                    globalMatrices[i] = localMatrix;
                }
                resolved[i] = true;
                changed = true;
            }
        }
    }

    return globalMatrices;
}

void OGAnimController::update(float deltaTime) {
    if (!m_current || m_current->duration <= 0.0f)
        return;

    m_currentTime += deltaTime;
    m_currentTime = fmod(m_currentTime, m_current->duration);

    if (m_blendTime < m_blendDuration)
        m_blendTime += deltaTime;
}

std::shared_ptr<OGAnimationClip> OGAnimController::getCurrentAnimation() const {
    return m_current;
}

std::shared_ptr<OGAnimationClip> OGAnimController::getPreviousAnimation() const {
    return m_previous;
}

void OGAnimController::playAnimation(std::shared_ptr<OGAnimationClip> clip) {

    if (m_current == clip)
        return;

    m_previous = m_current;
    m_current = clip;

    m_currentTime = 0.0f;
    m_blendTime = 0.0f;
    m_blendDuration = 0.2f;
}

std::vector<glm::mat4>
OGAnimController::getBlendedGlobalMatrices(const std::vector<OGJointInfo> &hierarchy,
                                           OGAnimationPose &pose) const {
    size_t numJoints = pose.joints.size();
    if (numJoints == 0) return std::vector<glm::mat4>();

    std::vector<glm::mat4> globalMatrices(numJoints);

    // Fast path: assume parents are always before children in the hierarchy
    // This is true for most animation exports (FBX, glTF, MD5)
    bool fastPathSucceeded = true;
    for (size_t i = 0; i < numJoints; ++i) {
        glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), pose.joints[i].position) *
                                glm::mat4_cast(pose.joints[i].orientation);

        int parentIdx = -1;
        if (i < hierarchy.size()) {
            parentIdx = hierarchy[i].parentIndex;
        }

        if (parentIdx < 0) {
            globalMatrices[i] = localMatrix;
        } else if (parentIdx < (int)i) {
            globalMatrices[i] = globalMatrices[parentIdx] * localMatrix;
        } else {
            // Out of order hierarchy detected, abort fast path
            fastPathSucceeded = false;
            break;
        }
    }

    if (fastPathSucceeded) {
        return globalMatrices;
    }

    // Slow path: iterative resolution for unordered hierarchies
    std::vector<bool> resolved(numJoints, false);
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < numJoints; ++i) {
            if (resolved[i]) continue;

            int parentIdx = -1;
            if (i < hierarchy.size()) {
                parentIdx = hierarchy[i].parentIndex;
            }

            if (parentIdx < 0 || (parentIdx < (int)numJoints && resolved[parentIdx])) {
                glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), pose.joints[i].position) *
                                        glm::mat4_cast(pose.joints[i].orientation);
                if (parentIdx >= 0) {
                    globalMatrices[i] = globalMatrices[parentIdx] * localMatrix;
                } else {
                    globalMatrices[i] = localMatrix;
                }
                resolved[i] = true;
                changed = true;
            }
        }
    }

    return globalMatrices;
}
