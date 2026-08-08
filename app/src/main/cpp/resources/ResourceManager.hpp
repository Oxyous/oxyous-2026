//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#ifndef OXYOUS_2026_RESOURCEMANAGER_HPP
#define OXYOUS_2026_RESOURCEMANAGER_HPP

#include <android/asset_manager.h>
#include "../includes.hpp"
#include "../system/OGSingleton.hpp"
#include "../render/vulkan/DescriptorCache.hpp"
#include <android/imagedecoder.h>
#include <future>
#include <mutex>

template<typename T>
class GPUResource {
public:
    GPUResource(const std::string& asset): m_assetPath(asset) { }

    virtual ~GPUResource() = default;

public:
    /** Destroy the resource */
    virtual void destroy() = 0;

    /** Load resource from asset */
    virtual bool load(AAssetManager *assetManager) = 0;

    /** Get Resource*/
    virtual T *get() = 0;

protected:
    std::string m_assetPath;
};

class ResourceManager {
public:
    ResourceManager();

    ~ResourceManager();

    /** Get Asset Manager */
    [[nodiscard]] virtual AAssetManager *getAssetManager() const;

    /** Set Asset Manager */
    void setAssetManager(AAssetManager *assetManager);

    /** Load Shader Binary form Assets*/
    void loadShader(const std::string &fileName, std::vector<uint8_t> &data);

    /** Load Scene collision */
    bool loadSceneCollision(const std::string& assetPath, std::vector<OGPolygon>& polys);

    /** Load Binary form Assets */
    template<typename T>
    static bool loadBinary(const std::string &assetPath, std::vector<uint8_t> &data) {
        auto resourceAsset = AAssetManager_open(m_assetManager, assetPath.c_str(),
                                                AASSET_MODE_BUFFER);
        if (resourceAsset == nullptr) {
            aout << "Failed to open asset: " << assetPath << std::endl;
            return false;
        }
        data.resize(AAsset_getLength(resourceAsset));
        AAsset_read(resourceAsset, data.data(), data.size());
        AAsset_close(resourceAsset);
        return true;
    }

    /** Read file as string */
    static bool readFileFromAssets(const std::string &assetPath, std::string& data) {
        auto resourceAsset = AAssetManager_open(m_assetManager, assetPath.c_str(), AASSET_MODE_BUFFER);

        if (resourceAsset == nullptr) {
            aout << "Failed to open asset:" << assetPath << std::endl;
            return false;
        }

        data.resize(AAsset_getLength(resourceAsset));
        AAsset_read(resourceAsset, data.data(), data.size());
        AAsset_close(resourceAsset);
        return true;
    }

    /** Load Texture From Assets */
    bool loadTextureData(const std::string &assetPath, std::vector<uint8_t>& data, uint32_t& size, uint32_t& width, uint32_t& height) {
        auto resourceAsset = AAssetManager_open(m_assetManager, assetPath.c_str(),
                                                AASSET_MODE_BUFFER);
        if (resourceAsset == nullptr) {
            aout << "Failed to open asset: " << assetPath << std::endl;
            return false;
        }

        AImageDecoder *pAndroidDecoder = nullptr;
        auto res = AImageDecoder_createFromAAsset(resourceAsset, &pAndroidDecoder);
        if (res != ANDROID_IMAGE_DECODER_SUCCESS) {
            aout << "Failed to create image decoder" << std::endl;
            return false;
        }

        AImageDecoder_setAndroidBitmapFormat(pAndroidDecoder, ANDROID_BITMAP_FORMAT_RGBA_8888);

        const AImageDecoderHeaderInfo *pAndroidHeader = nullptr;
        pAndroidHeader = AImageDecoder_getHeaderInfo(pAndroidDecoder);

        auto w = AImageDecoderHeaderInfo_getWidth(pAndroidHeader);
        auto h = AImageDecoderHeaderInfo_getHeight(pAndroidHeader);
        auto stride = AImageDecoder_getMinimumStride(pAndroidDecoder);

        auto upAndroidImageData = std::make_unique<std::vector<uint8_t>>(h * stride);

        auto decodeResults = AImageDecoder_decodeImage(pAndroidDecoder, upAndroidImageData->data(),
                                                       stride, upAndroidImageData->size());

        if (decodeResults != ANDROID_IMAGE_DECODER_SUCCESS) {
            aout << "Failed to decode image" << std::endl;
            return false;
        }

        AImageDecoder_delete(pAndroidDecoder);
        AAsset_close(resourceAsset);

        // upAndroidImageData already contains the decoded data.
        // We move it to the output vector to avoid a full copy if possible,
        // but since we need to match the signature, we'll just swap.
        data.swap(*upAndroidImageData);
        size = data.size();
        width = w;
        height = h;

        return true;
    }

public:
    /** Load Resource from Asset */
    template<typename T>
    static std::shared_ptr<T> load(const std::string &assetPath) {
        auto resource = std::make_shared<T>(assetPath);
        if (!resource->load(m_assetManager)) {
            aout << "Failed to load resource: " << assetPath << std::endl;
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(m_resourceMutex);
        m_resources<T>[assetPath] = resource;
        return resource;
    }

    /** Get or fetch (load) Resource */
    template<typename T>
    static std::shared_ptr<T> get(const std::string &assetPath) {
        {
            std::lock_guard<std::mutex> lock(m_resourceMutex);
            auto it = m_resources<T>.find(assetPath);
            if (it != m_resources<T>.end()) {
                return it->second;
            }
        }

        return load<T>(assetPath);
    }

    /** Get Resource Asynchronously */
    template<typename T>
    static std::future<std::shared_ptr<T>> getAsync(const std::string &assetPath) {
        return std::async(std::launch::async, [assetPath]() {
            return get<T>(assetPath);
        });
    }

private:
    inline static AAssetManager *m_assetManager = nullptr;
    inline static std::mutex m_resourceMutex;
private:
    template<typename T>
    inline static std::unordered_map<std::string, std::shared_ptr<T>> m_resources;

    inline static std::vector<void (*)()> m_clearCallbacks;
};

#define RESOURCE_MANAGER OGSingleton<ResourceManager>::getInstance()

#endif //OXYOUS_2026_RESOURCEMANAGER_HPP
