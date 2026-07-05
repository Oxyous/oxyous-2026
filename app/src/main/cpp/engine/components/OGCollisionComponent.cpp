//
// Created by Mr Steven J Baldwin on 04/07/2026.
//

#include "OGCollisionComponent.hpp"

void OGCollisionComponent::initialize() {

}

void OGCollisionComponent::update(double deltaTime) {

}

void OGCollisionComponent::destroy() {

}

void OGCollisionComponent::render(VkCommandBuffer &commandBuffer, uint64_t currentFrame) {

}

void OGCollisionComponent::setVolume(std::unique_ptr<IVolume> volume) {
    m_collider = std::move(volume);
}
