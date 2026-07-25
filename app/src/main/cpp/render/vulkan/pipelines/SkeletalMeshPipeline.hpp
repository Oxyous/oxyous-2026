//
// Created by Mr Steven J Baldwin on 25/07/2026.
//

#ifndef OXYOUS_2026_SKELETALMESHPIPELINE_HPP
#define OXYOUS_2026_SKELETALMESHPIPELINE_HPP


#include <stdint.h>
#include "../../../includes.hpp"
#include "IRenderPipeline.hpp"

class SkeletalMeshPipeline : public IRenderPipeline {

public:
    SkeletalMeshPipeline(IRenderPipeline *pipeline) : IRenderPipeline(pipeline) {
        m_parentPipeline = pipeline;
    }

    ~SkeletalMeshPipeline() override = default;

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

    IRenderPipeline* m_parentPipeline;

private:
    VkShaderModule m_vertexShaderModule;
    VkShaderModule m_fragmentShaderModule;
};


#endif //OXYOUS_2026_SKELETALMESHPIPELINE_HPP
