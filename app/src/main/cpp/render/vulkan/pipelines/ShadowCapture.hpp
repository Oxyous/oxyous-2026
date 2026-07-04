//
// Created by Mr Steven J Baldwin on 29/06/2026.
//

#ifndef OXYOUS_2026_SHADOWCAPTURE_HPP
#define OXYOUS_2026_SHADOWCAPTURE_HPP


#include "IRenderPipeline.hpp"
#include "../../../includes.hpp"
#include "../uniform/UniformBuffer.hpp"

class ShadowCapture : public IRenderPipeline {
public:
    ShadowCapture();

    ~ShadowCapture() override;
public:

    bool initialize() override;

    void destroy() override;

    void update(double delta) override;

    void execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore, const VkFence &fence) override;

    void resize(int width, int height) override;

    void render(VkCommandBuffer cmd, uint32_t currentFrame);

    void bindPipeline(VkCommandBuffer const &commandBuffer) override;

    void record(VkCommandBuffer commandBuffer, uint64_t currentFrame, VkFramebuffer framebuffer = VK_NULL_HANDLE) override;

    VkDescriptorImageInfo getFrameBufferImage(const std::string &name);

protected:
    virtual bool initializeRenderPass();

    virtual bool initializeFrameBuffer();

private:
    uint32_t m_shadowMapSize = 1024;
    VkSampler m_sampler;
    VkRenderPass m_renderPass;
    uint32_t m_cascadeCount = 4;
    VkImage m_shadowMapImage;
    VkDeviceMemory m_shadowMapMemory;
    VkImageView m_shadowMapImageView;

    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> frameBuffers;
    VkDescriptorImageInfo m_shadowMapDescriptor;
    VkShaderModule vertexShaderModule;
    UniformBuffer m_uniformBuffer;
    VkDescriptorSetLayout m_shadowDSL{};
    VkDescriptorSet m_shadowSets[2]{};
    VkDescriptorPool m_descriptorPool{};
};


#endif //OXYOUS_2026_SHADOWCAPTURE_HPP
