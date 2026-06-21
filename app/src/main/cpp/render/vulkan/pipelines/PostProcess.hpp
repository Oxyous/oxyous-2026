//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_POSTPROCESS_HPP
#define OXYOUS_2026_POSTPROCESS_HPP

#include "../../../includes.hpp"
#include "IRenderPipeline.hpp"
#include "../../../DataStructures.hpp"
#include "../uniform/UniformBuffer.hpp"

class PostProcess : public IRenderPipeline {
public:
    PostProcess();

    ~PostProcess() override;

    /*  Update Render Pipeline */
    void update(double delta) override;

    /* Execute Render Pipeline */
    void execute(const VkSemaphore &waitSemaphore, const VkSemaphore &signalSemaphore,
                 const VkFence &fence) override;

    /* Initialize Render Pipeline */
    bool initialize() override;

    /* Destroy Render Pipeline */
    void destroy() override;

    /* Render Pipeline */
    void resize(int width, int height) override;

    /* Bind Pipeline */
    void bindPipeline(VkCommandBuffer const &commandBuffer) override;

    /* Update Descriptors */
    virtual void updateDescriptors();

    /* Set Frame Buffer Image */
    void setFrameBufferImage(const std::string& name, const VkDescriptorImageInfo& image);

    /* Record Command Buffer */
    void record(VkCommandBuffer commandBuffer, uint64_t currentFrame, VkFramebuffer framebuffer = VK_NULL_HANDLE) override;

protected:
    /* Initialize Render Pass */
    virtual bool initializeRenderPass();

    /* Initialize Descriptors */
    virtual bool initializeDescriptors();
protected:
    std::unordered_map<std::string, VkDescriptorImageInfo> m_frameBufferImages;
    VkShaderModule m_vertShaderModule{};
    VkShaderModule m_fragShaderModule{};
    VkDescriptorSetLayout m_perFrameDSL{};
    VkDescriptorPool m_descriptorPool{};
    VkDescriptorSet m_descriptorSet{};
    uint32_t m_width{};
    uint32_t m_height{};
    UniformBuffer m_uniformBuffer{};
};


#endif //OXYOUS_2026_POSTPROCESS_HPP
