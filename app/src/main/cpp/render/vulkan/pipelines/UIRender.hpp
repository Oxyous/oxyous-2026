//
// Created by Mr Steven J Baldwin on 08/07/2026.
//

#ifndef OXYOUS_2026_UIRENDER_HPP
#define OXYOUS_2026_UIRENDER_HPP

#include "IRenderPipeline.hpp"
#include "DataStructures.hpp"



class UIRender : public IRenderPipeline {
public:
    UIRender() = default;

    void update(double delta) override;

    void execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                 const VkFence &fence) override;

    void destroy() override;

    void resize(int width, int height) override;

    void bindPipeline(VkCommandBuffer const &commandBuffer) override;

    /** */
    bool initialize() override;

    /** */
    void updateInstances();

    /** */
    void record(VkCommandBuffer cmd, uint64_t currentFrame, VkFramebuffer framebuffer) override;

    /** */
    void setRenderPass(VkRenderPass renderPass) override;

    /** */
    void updateDescriptorSet();

private:

    constexpr static int MAX_SPRITES = 1000;


    bool initializeRenderPass();

    bool initializeDescriptorPool();

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkShaderModule m_vertShaderModule, m_fragShaderModule;

    GPUBuffer m_vertexBuffer;
    GPUBuffer m_indexBuffer;
    GPUBuffer m_instanceBuffer;
};


#endif //OXYOUS_2026_UIRENDER_HPP
