//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_GPUTEXTURERESOURCE_HPP
#define OXYOUS_2026_GPUTEXTURERESOURCE_HPP

#include "../DataStructures.hpp"
#include "ResourceManager.hpp"

enum TEXTURE_SLOT {
    TEXTURE_SLOT_0,
    TEXTURE_SLOT_1,
    TEXTURE_SLOT_2,
    TEXTURE_SLOT_3,
    TEXTURE_SLOT_4,
    TEXTURE_SLOT_5,
    TEXTURE_SLOT_6,
    TEXTURE_SLOT_7
};

class GPUTextureResource: public GPUResource<GPUTexture> {
public:
    GPUTextureResource(const std::string& assetPath): GPUResource<GPUTexture>(assetPath) {}
    virtual ~GPUTextureResource() = default;

    /* */
    bool load(AAssetManager *assetManager, const std::vector<uint8_t> &data) override;

    /* */
    virtual GPUTexture* get() override;

    /* */
    virtual void destroy() override;

private:
    std::shared_ptr<GPUTexture> m_texture;
};


#endif //OXYOUS_2026_GPUTEXTURERESOURCE_HPP
