//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#ifndef OXYOUS_2026_DEFERRED_HPP
#define OXYOUS_2026_DEFERRED_HPP

#include "IRenderPipeline.hpp"
#include "../../../includes.hpp"
#include "../../../DataStructures.hpp"

class Deferred: public IRenderPipeline{
public:
    Deferred();

    /* */
    ~Deferred() override;

    /* */
    void update() override;

    /* */
    void execute(const VkSemaphore& waitSemaphore, const VkSemaphore& signalSemaphore, const VkFence& fence) override;

    /* */
    bool initialize() override;

    /* Set render pass from parent (Renderer) */
    void setRenderPass(VkRenderPass renderPass) { m_renderPass = renderPass; }

    /* */
    void destroy() override;

    /* */
    void resize(int width, int height) override;

    /* */
    void bindPipeline(const VkCommandBuffer& commandBuffer);

protected:
    std::unordered_map<std::string, GPUImage> m_frameBufferImages;
    VkFramebuffer m_frameBuffer;
    VkRenderPass m_renderPass;
    VkShaderModule m_vertShaderModule;
    VkShaderModule m_fragShaderModule;
    VkPipelineLayout m_pipelineLayout;
    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;
};


#endif //OXYOUS_2026_DEFERRED_HPP
