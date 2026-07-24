//
// Created by Mr Steven J Baldwin on 21/07/2026.
//

#ifndef OXYOUS_2026_OGANIMATIONMANAGER_HPP
#define OXYOUS_2026_OGANIMATIONMANAGER_HPP

#include "../../includes.hpp"
#include "DataStructures.hpp"
#include "../../system/OGSingleton.hpp"

class OGAnimationManager {
public:
    OGAnimationManager() = default;
    ~OGAnimationManager() = default;

    /** Load Animation */
    bool loadAnimation(const std::string& name, const std::string& assetPath);

    /** Get Animation Clip by Name */
    OGAnimationClip* getAnimation(const std::string& name);

    /** Interpolate between two skeleton frames */
    void interlopeSkeletons(OGFrameSkeleton& outSkeleton,
                            const OGFrameSkeleton& skeletonA,
                            const OGFrameSkeleton& skeletonB, float t);
private:
    /** Build individual frame skeleton */
    void buildFrameSkeleton(std::vector<OGFrameSkeleton>& frameSkeletons,
                            const std::vector<OGJointInfo> &joints,
                            const std::vector<OGJointTransform> &baseFrames,
                            const OGAnimFrame &frameData);
private:
    std::unordered_map<std::string, std::shared_ptr<OGAnimationClip>> m_animations;
};

#define ANIMATION_MANAGER OGSingleton<OGAnimationManager>::getInstance()

#endif //OXYOUS_2026_OGANIMATIONMANAGER_HPP
