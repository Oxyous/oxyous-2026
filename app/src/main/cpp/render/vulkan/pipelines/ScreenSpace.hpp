//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#ifndef OXYOUS_2026_SCREENSPACE_HPP
#define OXYOUS_2026_SCREENSPACE_HPP


#include "IRenderPipeline.hpp"
#include "../../../includes.hpp"

class ScreenSpace: public IRenderPipeline {
public:
    ScreenSpace() = default;

    void update(double delta) override;

    bool initialize() override;

    void execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                 const VkFence &fence) override;

    void destroy() override;

    void resize(int width, int height) override;

    void record(VkCommandBuffer commandBuffer, uint64_t currentFrame,
                VkFramebuffer framebuffer) override;

    void setRenderPass(VkRenderPass renderPass) override;

    void bindPipeline(VkCommandBuffer const &commandBuffer) override;

    VkPipelineLayout getPipelineLayout() const override;

protected:
    bool initializeRenderPass();

    VkShaderModule m_vertShaderModule{};
    VkShaderModule m_fragShaderModule{};
    VkDescriptorSetLayout m_perFrameDSL{};
};


#endif //OXYOUS_2026_SCREENSPACE_HPP
