//
// Created by Mr Steven J Baldwin on 20/07/2026.
//

#ifndef OXYOUS_2026_OGSKELETALANIMATION_HPP
#define OXYOUS_2026_OGSKELETALANIMATION_HPP

#include "../../DataStructures.hpp"
#include "../ResourceManager.hpp"
#include "../../engine/math/MathHelper.hpp"

class OGSkeletalAnimation {
public:
    OGSkeletalAnimation() = default;

    ~OGSkeletalAnimation() = default;

    friend class OGSkeletalAnimationResource;

protected:

    /** */
    OGAnimationClip* getAnimationClip() {
        return m_animationClip.get();
    }

    OGFrameSkeleton* getAnimatedSkeleton() {
        return m_animatedSkeleton.get();
    }

protected:
    std::shared_ptr<OGAnimationClip> m_animationClip;
    std::shared_ptr<OGFrameSkeleton> m_animatedSkeleton;
};

/** GPU Skeletal Animation Resource */
class OGSkeletalAnimationResource : public GPUResource<OGSkeletalAnimation> {
public:
    OGSkeletalAnimationResource(const std::string &asset) : GPUResource<OGSkeletalAnimation>(
            asset) {

    }

    OGSkeletalAnimation *get() override {
        return m_animation.get();
    }

    bool load(AAssetManager *assetManager, const std::vector<uint8_t> &data) override;

private:
    std::shared_ptr<OGSkeletalAnimation> m_animation;
};


#endif //OXYOUS_2026_OGSKELETALANIMATION_HPP
