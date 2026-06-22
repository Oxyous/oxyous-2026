//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#ifndef OXYOUS_2026_DEFERRED_HPP
#define OXYOUS_2026_DEFERRED_HPP

#include "IRenderPipeline.hpp"
#include "../../../includes.hpp"
#include "../../../DataStructures.hpp"
#include "../uniform/UniformBuffer.hpp"

class Deferred : public IRenderPipeline {
public:
    Deferred();

    /* */
    ~Deferred() = default;

    /* */
    void update(double delta) override;

    /* */
    void execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                         const VkFence &fence) override;

    /* */
    bool initialize() override;

    /* */
    void destroy() override;

    /* */
    void resize(int width, int height) override;

    /* */
    void bindPipeline(const VkCommandBuffer &commandBuffer) override;

    /* Get Frame Buffer image*/
    virtual VkDescriptorImageInfo *getFrameBufferImage(const std::string &name);

    /* Record Command buffer */
    void record(VkCommandBuffer commandBuffer, uint64_t currentFrame, VkFramebuffer framebuffer = VK_NULL_HANDLE) override;
protected:
    /* Initialize Render Pass */
    virtual bool initializeRenderPass();

    /* Initialize Frame Buffers */
    virtual bool initializeFramebuffers();

protected:
    std::unordered_map<std::string, GPUTexture> m_frameBufferImages;
    VkShaderModule m_vertShaderModule;
    VkShaderModule m_fragShaderModule;
    uint32_t m_width;
    uint32_t m_height;
    VkFramebuffer m_frameBuffer;
    VkSampler m_sampler;
    VkCommandBuffer m_commandBuffer {VK_NULL_HANDLE};
};


#endif //OXYOUS_2026_DEFERRED_HPP
