//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "GPUTextureResource.hpp"
#include "../render/vulkan/RenderFramework.hpp"
#include <android/imagedecoder.h>

bool GPUTextureResource::load(AAssetManager *assetManager, const std::vector<uint8_t> &data) {

    auto textureAsset = AAssetManager_open(assetManager, m_assetPath.c_str(), AASSET_MODE_BUFFER);
    if (textureAsset == nullptr) {
        aout << "Failed to open texture asset: " << m_assetPath << std::endl;
        return false;
    }

    AImageDecoder *pAndroidDecoder = nullptr;
    auto res = AImageDecoder_createFromAAsset(textureAsset, &pAndroidDecoder);
    if (res != ANDROID_IMAGE_DECODER_SUCCESS) {
        aout << "Failed to create image decoder" << std::endl;
        return false;
    }

    AImageDecoder_setAndroidBitmapFormat(pAndroidDecoder, ANDROID_BITMAP_FORMAT_RGBA_8888);

    const AImageDecoderHeaderInfo *pAndroidHeader = nullptr;
    pAndroidHeader = AImageDecoder_getHeaderInfo(pAndroidDecoder);

    auto width = AImageDecoderHeaderInfo_getWidth(pAndroidHeader);
    auto height = AImageDecoderHeaderInfo_getHeight(pAndroidHeader);
    auto stride = AImageDecoder_getMinimumStride(pAndroidDecoder);

    auto upAndroidImageData = std::make_unique<std::vector<uint8_t>>(height * stride);

    auto decodeResults = AImageDecoder_decodeImage(pAndroidDecoder, upAndroidImageData->data(),
                                                   stride, upAndroidImageData->size());

    if (decodeResults != ANDROID_IMAGE_DECODER_SUCCESS) {
        aout << "Failed to decode image" << std::endl;
        return false;
    }

    AImageDecoder_delete(pAndroidDecoder);
    AAsset_close(textureAsset);
    m_texture = std::make_unique<GPUTexture>();
    if (!RenderFramework::createGpuTexture(upAndroidImageData->data(), upAndroidImageData->size(), VK_FORMAT_R8G8B8A8_UNORM, width, height, m_texture)) {
        aout << "Failed to create gpu texture" << std::endl;
        return false;
    }
    return true;
}

GPUTexture *GPUTextureResource::get() {
    return m_texture.get();
}

void GPUTextureResource::destroy() {

}
