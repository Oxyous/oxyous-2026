//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#include "OGStaticMeshComponent.hpp"
#include "../../render/vulkan/DescriptorCache.hpp"
#include "../Engine.hpp"
#include "../../render/vulkan/pipelines/Deferred.hpp"
#include "../GPUResources.hpp"

/* set mesh resource */
void OGStaticMeshComponent::setMeshResource(std::shared_ptr<GPUStaticMeshResource> mesh) {
    m_mesh = std::move(mesh);
}

/* set texture resource */
void OGStaticMeshComponent::setTextureResource(TEXTURE_SLOT slot, std::shared_ptr<GPUTextureResource> texture) {
    m_textures[slot] = std::move(texture);
}

void OGStaticMeshComponent::initialize() {
    GPUMeshHandle data = {};
    data.model = glm::mat4(1.0f);
    m_objectIndex = GPU_RESOURCES->registerObject(data);
}

void OGStaticMeshComponent::update(double deltaTime) {
    if (m_owner) {
        glm::mat4 worldTransform = m_owner->getWorldTransform();

        GPUMeshHandle data = {};
        data.model = worldTransform;
        data.materialIndex = m_materialIndex;
        GPU_RESOURCES->updateObject(m_objectIndex, data);
    }
}

void OGStaticMeshComponent::destroy() {

}

void OGStaticMeshComponent::render(VkCommandBuffer &commandBuffer, uint64_t currentFrame) {
    if (!m_mesh || !m_mesh->get())
        return;

    BindlessPushConstants pc = {};
    pc.materialIndex = m_materialIndex;
    pc.objectIndex = m_objectIndex;
    vkCmdPushConstants(commandBuffer, GPU_RESOURCES->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BindlessPushConstants), &pc);

    m_mesh->get()->render(commandBuffer);
}

void OGStaticMeshComponent::setMaterialIndex(uint32_t index) {
    m_materialIndex = index;
}
