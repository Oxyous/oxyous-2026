//
// Created by Mr Steven J Baldwin on 18/06/2026.
//

#ifndef OXYOUS_2026_IRENDERPIPELINE_HPP
#define OXYOUS_2026_IRENDERPIPELINE_HPP


#include <vulkan/vulkan.h>

class IRenderPipeline {
public:
    /* */
    IRenderPipeline(IRenderPipeline* pipeline = nullptr) { };

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

    /** Get Render Pass*/
    virtual VkRenderPass getRenderPass() const { return m_renderPass; }

    /** */
    virtual VkPipelineViewportStateCreateInfo* getViewportState() { return &m_viewportState; }

    /** */
    virtual VkPipelineInputAssemblyStateCreateInfo* getInputAssemblyState() { return &m_inputAssembly; }

    /** */
    virtual VkPipelineRasterizationStateCreateInfo* getRasterizationState() { return &m_rasterization; }

    /** */
    virtual VkPipelineMultisampleStateCreateInfo* getMultisampleState() { return &m_multisampling; }

    /** */
    virtual VkPipelineDepthStencilStateCreateInfo* getDepthStencilState() { return &m_depthStencil; }

    /** */
    virtual VkPipelineColorBlendStateCreateInfo* getColorBlendState() { return &m_colorBlending; }

    /** */
    virtual VkPipelineDynamicStateCreateInfo* getDynamicState() { return &m_dynamicState; }

    /** */
    virtual VkPipelineVertexInputStateCreateInfo* getVertexInputState() { return &m_vertexInput; }

protected:
    VkCommandBuffer m_commandBuffer{};
    VkPipeline m_pipeline{};
    VkPipelineLayout m_pipelineLayout{};
    VkRenderPass m_renderPass{};
    VkPipelineViewportStateCreateInfo m_viewportState{};
    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};
    VkPipelineRasterizationStateCreateInfo m_rasterization{};
    VkPipelineMultisampleStateCreateInfo m_multisampling{};
    VkPipelineDepthStencilStateCreateInfo m_depthStencil{};
    VkPipelineColorBlendStateCreateInfo m_colorBlending{};
    VkPipelineDynamicStateCreateInfo m_dynamicState{};
    VkPipelineVertexInputStateCreateInfo m_vertexInput{};

};


#endif //OXYOUS_2026_IRENDERPIPELINE_HPP
