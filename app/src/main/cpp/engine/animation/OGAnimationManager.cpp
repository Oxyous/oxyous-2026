//
// Created by Mr Steven J Baldwin on 21/07/2026.
//

#include "OGAnimationManager.hpp"
#include "resources/ResourceManager.hpp"

bool OGAnimationManager::loadAnimation(const std::string &name, const std::string &assetPath) {

    auto animationAsset = AAssetManager_open(RESOURCE_MANAGER->getAssetManager(), assetPath.c_str(),
                                             AASSET_MODE_BUFFER);

    if (!animationAsset) {
        aout << "Failed to open animation asset: " << assetPath << std::endl;
        return false;
    }

    auto animationData = AAsset_getBuffer(animationAsset);
    auto animationSize = AAsset_getLength(animationAsset);
    auto dataPtr = static_cast<const uint8_t *>(animationData);

    OGAnimHeader *header = reinterpret_cast<OGAnimHeader *>(const_cast<uint8_t *>(dataPtr));
    dataPtr += sizeof(OGAnimHeader);

    dataPtr = static_cast<const uint8_t *>(animationData);
    dataPtr += header->hierarchyOffset;

    OGAnimationClip animClip;


    for (int i = 0; i < header->hierarchyCount; i++) {
        OGJointInfo jointInfo;
        uint32_t nameLength;

        nameLength = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        jointInfo.name.resize(nameLength);
        std::memcpy(jointInfo.name.data(), dataPtr, nameLength);
        dataPtr += nameLength;

        jointInfo.parentIndex = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        jointInfo.flags = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        jointInfo.startIndex = *reinterpret_cast<const uint32_t *>(dataPtr);
        dataPtr += sizeof(uint32_t);

        animClip.joints.push_back(jointInfo);
    }

    dataPtr = static_cast<const uint8_t *>(animationData);
    dataPtr += header->baseFrameOffset;

    for (int i = 0; i < header->baseFrameCount; i++) {
        OGJointTransform frame{};
        float orientation[3] = {0.0f, 0.0f, 0.0f};

        frame.position = *reinterpret_cast<const glm::vec3 *>(dataPtr);
        dataPtr += sizeof(glm::vec3);

        orientation[0] = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        orientation[1] = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        orientation[2] = *reinterpret_cast<const float *>(dataPtr);
        dataPtr += sizeof(float);

        frame.orientation = glm::quat(0.0f, orientation[0], orientation[1], orientation[2]);

        Math::computeQuaternion(frame.orientation);

        animClip.baseFrames.push_back(frame);
    }

    dataPtr = static_cast<const uint8_t *>(animationData);
    dataPtr += header->framesOffset;
    int32_t frameNo = 0;
    int32_t frameDataSize = 0;

    for (int i = 0; i < header->framesCount; i++) {
        OGAnimFrame frameData;

        frameNo = *reinterpret_cast<const int *>(dataPtr);
        dataPtr += sizeof(int32_t);

        frameDataSize = *reinterpret_cast<const int *>(dataPtr);
        dataPtr += sizeof(int32_t);

        frameData.iFrameID = frameNo;

        for (int j = 0; j < frameDataSize; j++) {
            float component = *reinterpret_cast<const float *>(dataPtr);
            dataPtr += sizeof(float);
            frameData.components.push_back(component);
        }

        buildFrameSkeleton(animClip.skeletonFrames, animClip.joints,
                           animClip.baseFrames, frameData);
    }


    AAsset_close(animationAsset);

    m_animations[name] = std::make_shared<OGAnimationClip>(animClip);
    m_animations[name]->getAnimationClip()->duration = 1.0f / header->frameRate;
    m_animations[name]->getAnimationClip()->numberFrames = header->numOfFrames;
    m_animations[name]->getAnimationClip()->numberJoints = header->numOfJoints;
    m_animations[name]->getAnimationClip()->frameRate = header->frameRate;

    return true;
}

OGAnimationClip *OGAnimationManager::getAnimation(const std::string &name) {
    auto it = m_animations.find(name);

    if (it != m_animations.end()) {
        return it->second.get();
    }
    return nullptr;
}

void OGAnimationManager::buildFrameSkeleton(std::vector<OGFrameSkeleton> &frameSkeletons,
                                            const std::vector<OGJointInfo> &joints,
                                            const std::vector<OGJointTransform> &baseFrames,
                                            const OGAnimFrame &frameData) {
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

/***/
void OGAnimationManager::interlopeSkeletons(OGFrameSkeleton &outSkeleton,
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
