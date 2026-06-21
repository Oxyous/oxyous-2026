//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#include "OGStaticMeshComponent.hpp"

/* set mesh resource */
void OGStaticMeshComponent::setMeshResource(std::shared_ptr<GPUStaticMeshResource> mesh) {
    m_mesh = std::move(mesh);
}

/* set texture resource */
void OGStaticMeshComponent::setTextureResource(TEXTURE_SLOT slot, std::shared_ptr<GPUTextureResource> texture) {
    m_textures[slot] = std::move(texture);
}

void OGStaticMeshComponent::initialize() {

}

void OGStaticMeshComponent::update(double deltaTime) {

}

void OGStaticMeshComponent::destroy() {

}

void OGStaticMeshComponent::render(VkCommandBuffer &commandBuffer) {
    const auto& staticMesh = m_mesh->get();
    const VkDeviceSize offsets[] = {0};

    if (staticMesh) {
        staticMesh->render(commandBuffer);
    }
}
