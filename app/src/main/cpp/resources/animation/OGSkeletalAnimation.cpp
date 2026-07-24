//
// Created by Mr Steven J Baldwin on 20/07/2026.
//

#include "OGSkeletalAnimation.hpp"

/** Build Frame Skeleton from animation */
void OGSkeletalAnimation::buildFrameSkeleton(std::vector<OGFrameSkeleton> &frameSkeletons,
                                             const std::vector<OGJointInfo> &joints,
                                             const std::vector<OGJointTransform> &baseFrame,
                                             const OGAnimFrame& frame) {
    OGFrameSkeleton skeleton;

    for (int i = 0; i < joints.size(); i++) {
        uint32_t j = 0;

        auto &joint = joints[i];

        OGJoint animJoint;
        animJoint.position = baseFrame[i].position;
        animJoint.orientation = baseFrame[i].orientation;
        animJoint.name = joint.name;

        animJoint.parentIndex = joint.parentIndex;

        if (joint.flags & 1) {
            animJoint.position.x = frame.components[joint.startIndex + j++];
        }

        if (joint.flags & 2) {
            animJoint.position.y = frame.components[joint.startIndex + j++];
        }

        if (joint.flags & 4) {
            animJoint.position.z = frame.components[joint.startIndex + j++];
        }

        if (joint.flags & 8) {
            animJoint.orientation.x = frame.components[joint.startIndex + j++];
        }

        if (joint.flags & 16) {
            animJoint.orientation.y = frame.components[joint.startIndex + j++];
        }

        if (joint.flags & 32) {
            animJoint.orientation.z = frame.components[joint.startIndex + j++];
        }

        Math::computeQuaternion(animJoint.orientation);

        if (animJoint.parentIndex >= 0) {
            OGJoint &parent = skeleton.joints[animJoint.parentIndex];
            glm::vec3 rotatedPosition = parent.orientation * animJoint.position;
            animJoint.position = parent.position + rotatedPosition;
            animJoint.orientation = parent.orientation * animJoint.orientation;

            animJoint.orientation = glm::normalize(animJoint.orientation);
        }

        skeleton.joints.push_back(animJoint);
    }

    frameSkeletons.push_back(skeleton);
}

void OGSkeletalAnimation::interlopeSkeletons(OGFrameSkeleton &outSkeleton,
                                             const OGFrameSkeleton &skeletonA,
                                             const OGFrameSkeleton &skeletonB, float t) {
    for (int i = 0; i < m_animationClip->numberJoints; i++) {
        auto &finalJoint = outSkeleton.joints[i];
        auto &finalMatrix = outSkeleton.transforms[i];

        const auto &jointA = skeletonA.joints[i];
        const auto &jointB = skeletonB.joints[i];

        finalJoint.parentIndex = jointA.parentIndex;

        // to check
        finalJoint.position = glm::mix(jointA.position, jointB.position, t);
        finalJoint.orientation = glm::slerp(jointA.orientation, jointB.orientation, t);

        finalMatrix = glm::translate(glm::mat4(1.0f), finalJoint.position) * glm::mat4_cast(finalJoint.orientation);

    }
}

bool
OGSkeletalAnimationResource::load(AAssetManager *assetManager, const std::vector<uint8_t> &data)  {

    return true;
}
