//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#ifndef OXYOUS_2026_SCREENSPACERENDERER_HPP
#define OXYOUS_2026_SCREENSPACERENDERER_HPP


#include "../../DataStructures.hpp"
#include "../../system/OGSingleton.hpp"

class ScreenSpaceRenderer {
public:
    bool initialize();

    VkDescriptorSet &getBindLessSet(uint32_t frame);

    VkPipelineLayout &getPipelineLayout() { return m_bindlessRenderer.pipelineLayout; }

    /** */
    void uploadFrameData(ScreenElements& frameData);

    /** */
    ScreenElements& getScreenElements(uint32_t frame) { return m_bindlessRenderer.frameData[frame]; }

    /** */
    uint32_t registerTexture(GPUTexture texture);

    /** */
    uint32_t registerElement(GPUElementHandle element);

    /** */
    void updateElement(uint32_t index, GPUElementHandle element);


    /** register object for bindless */
    uint32_t registerObject(GPUElementHandle object);

    /** Create Screen Quad */
    GPUBuffer createQuadBuffer(const glm::vec2& origin, const glm::vec2& size);

protected:
    /** */
    uint32_t allocateTextureSlot();

    bool createBindlessDescriptors();

    bool createPipelineLayout();

    bool initializeFrames();

    bool initializeFrame(ScreenElements& frame);

protected:
    ScreenElements m_screenElements;
    BindlessRenderer2D m_bindlessRenderer;
};

#define SCREEN_RENDER OGSingleton<ScreenSpaceRenderer>::getInstance()


#endif //OXYOUS_2026_SCREENSPACERENDERER_HPP
