//
// Created by Mr Steven J Baldwin on 21/07/2026.
//

#include "OGAnimationManager.hpp"
#include "resources/ResourceManager.hpp"
#include "../../engine/math/MathHelper.hpp"

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
        animClip.jointInfos.push_back(jointInfo);
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

    auto clip = std::make_shared<OGAnimationClip>(animClip);
    clip->name = name;
    clip->duration = (float)(header->numOfFrames > 1 ? header->numOfFrames - 1 : 1) / header->frameRate;
    clip->numberFrames = header->numOfFrames;
    clip->numberJoints = header->numOfJoints;
    clip->frameRate = header->frameRate;
    clip->jointInfos = animClip.jointInfos;

    m_animations[name] = clip;

    aout << "Loaded animation: " << name << " with " << clip->skeletonFrames.size() << " frames, duration: " << clip->duration << std::endl;

    return true;
}

std::shared_ptr<OGAnimationClip> OGAnimationManager::getAnimation(const std::string &name) {
    auto it = m_animations.find(name);

    if (it != m_animations.end()) {
        return it->second;
    }
    return nullptr;
}

void OGAnimationManager::buildFrameSkeleton(std::vector<OGFrameSkeleton> &frameSkeletons,
                                            const std::vector<OGJointInfo> &joints,
                                            const std::vector<OGJointTransform> &baseFrame,
                                            const OGAnimFrame &frameData) {
    OGFrameSkeleton skeleton;
    size_t numJoints = joints.size();
    skeleton.joints.reserve(numJoints);
    skeleton.transforms.resize(numJoints);

    for (int i = 0; i < numJoints; i++) {
        uint32_t j = 0;

        auto &joint = joints[i];

        OGJoint animJoint;
        if (i < (int)baseFrame.size()) {
            animJoint.position = baseFrame[i].position;
            animJoint.orientation = baseFrame[i].orientation;
        } else {
            animJoint.position = glm::vec3(0.0f);
            animJoint.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        animJoint.name = joint.name;
        animJoint.parentIndex = joint.parentIndex;

        size_t componentsSize = frameData.components.size();
        if (joint.flags & 1 && (size_t)joint.startIndex + j < componentsSize) {
            animJoint.position.x = frameData.components[joint.startIndex + j++];
        }

        if (joint.flags & 2 && (size_t)joint.startIndex + j < componentsSize) {
            animJoint.position.y = frameData.components[joint.startIndex + j++];
        }

        if (joint.flags & 4 && (size_t)joint.startIndex + j < componentsSize) {
            animJoint.position.z = frameData.components[joint.startIndex + j++];
        }

        if (joint.flags & 8 && (size_t)joint.startIndex + j < componentsSize) {
            animJoint.orientation.x = frameData.components[joint.startIndex + j++];
        }

        if (joint.flags & 16 && (size_t)joint.startIndex + j < componentsSize) {
            animJoint.orientation.y = frameData.components[joint.startIndex + j++];
        }

        if (joint.flags & 32 && (size_t)joint.startIndex + j < componentsSize) {
            animJoint.orientation.z = frameData.components[joint.startIndex + j++];
        }

        Math::computeQuaternion(animJoint.orientation);

        // Store as LOCAL transform (relative to parent)
        skeleton.joints.push_back(animJoint);

        // Also compute a global transform for this frame if needed later,
        // but for now we primarily want local joints for blending.
        // We'll compute the global matrices in OGAnimController.
    }

    frameSkeletons.push_back(skeleton);
}

/***/
void OGAnimationManager::interlopeSkeletons(OGFrameSkeleton &outSkeleton,
                                            const OGFrameSkeleton &skeletonA,
                                            const OGFrameSkeleton &skeletonB, float t) {
    for (int i = 0; i < outSkeleton.joints.size(); i++) {
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
