//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#ifndef OXYOUS_2026_IRENDERPIPELINE_HPP
#define OXYOUS_2026_IRENDERPIPELINE_HPP


#include <vulkan/vulkan.h>

class IRenderPipeline {
public:
    /* */
    IRenderPipeline() = default;

    /* */
    virtual ~IRenderPipeline() = default;

    /* */
    virtual void update(double delta) = 0;

    /* */
    virtual bool initialize() = 0;

    /* */
    virtual void execute(const VkSemaphore& waitSemaphore, const VkSemaphore& signalSemaphore, const VkFence& fence) = 0;

    /* */
    virtual void destroy() = 0;

    /* */
    virtual void resize(int width, int height) = 0;

    /* Record Command  */
    virtual void record(VkCommandBuffer commandBuffer, uint64_t currentFrame, VkFramebuffer framebuffer = VK_NULL_HANDLE) = 0;

    /* Set render pass */
    virtual void setRenderPass(VkRenderPass renderPass) { m_renderPass = renderPass; }

    /* */
    virtual void bindPipeline(VkCommandBuffer const& commandBuffer) = 0;

    /* Get pipeline layout */
    virtual VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
protected:
    VkCommandBuffer m_commandBuffer{};
    VkPipeline m_pipeline{};
    VkPipelineLayout m_pipelineLayout{};
    VkRenderPass m_renderPass{};
};


#endif //OXYOUS_2026_IRENDERPIPELINE_HPP
