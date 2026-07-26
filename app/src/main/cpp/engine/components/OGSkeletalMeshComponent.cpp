//
// Created by Mr Steven J Baldwin on 25/07/2026.
//

#include <stdint.h>
#include "OGSkeletalMeshComponent.hpp"
#include "engine/GPUResources.hpp"

OGSkeletalMeshComponent::OGSkeletalMeshComponent() {

}

OGSkeletalMeshComponent::~OGSkeletalMeshComponent() {

}

void OGSkeletalMeshComponent::initialize() {
    m_boneIndex = GPU_RESOURCES->registerBoneBlock();
    GPUMeshHandle data = {};
    data.model = glm::mat4(1.0f);
    data.materialIndex = m_materialIndex;
    data.boneIndex = m_boneIndex;
    m_objectIndex = GPU_RESOURCES->registerObject(data);
}

void OGSkeletalMeshComponent::update(double deltaTime) {
    if (m_owner && m_objectIndex != 0xFFFFFFFF) {
        GPUMeshHandle data = {};
        data.model = m_owner->getWorldTransform();
        data.materialIndex = m_materialIndex;
        data.boneIndex = m_boneIndex;
        GPU_RESOURCES->updateObject(m_objectIndex, data);
    }
}

void OGSkeletalMeshComponent::destroy() {

}

void OGSkeletalMeshComponent::render(VkCommandBuffer &commandBuffer, uint64_t currentFrame) {
    if (!m_mesh || !m_mesh->get()) return;

    const auto &mesh = m_mesh->getSkeletalMesh();

    BindlessPushConstants pc = {};
    pc.materialIndex = m_materialIndex;
    pc.objectIndex = m_objectIndex;

    vkCmdPushConstants(commandBuffer, GPU_RESOURCES->getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(BindlessPushConstants), &pc);

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh->getVertexBuffer()->buffer, &mesh->getVertexBuffer()->offset);
    vkCmdBindIndexBuffer(commandBuffer, mesh->getIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, *mesh->getIndexCount(), 1, 0, 0, 0);
}

void OGSkeletalMeshComponent::setMaterialIndex(uint32_t index) {
    m_materialIndex = index;
}

void OGSkeletalMeshComponent::renderShadow(VkCommandBuffer &commandBuffer, uint64_t currentFrame, VkPipelineLayout layout, CSMData data, uint32_t cascade) {
    if (!m_mesh || !m_mesh->get()) return;

    const auto &mesh = m_mesh->getSkeletalMesh();

    ShadowMapPushConstants pc = {};
    pc.objectIndex = m_objectIndex;
    pc.cascadeIndex = cascade;

    vkCmdPushConstants(commandBuffer, layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(ShadowMapPushConstants), &pc);

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh->getVertexBuffer()->buffer, &mesh->getVertexBuffer()->offset);
    vkCmdBindIndexBuffer(commandBuffer, mesh->getIndexBuffer()->buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, *mesh->getIndexCount(), 1, 0, 0, 0);
}

OGEntity *OGSkeletalMeshComponent::getOwner() const {
    return OGComponent::getOwner();
}

void OGSkeletalMeshComponent::setMeshResource(const std::shared_ptr<GPUSkeletalMeshResource> &mesh) {
    this->m_mesh = mesh;
}
