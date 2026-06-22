//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "Engine.hpp"
#include "../render/vulkan/DescriptorCache.hpp"

bool Engine::initialize(android_app* app) {
    m_app = app;

    return true;
}

void Engine::destroy() {

}

void Engine::render() {

}

void Engine::update(float deltaTime) {

}

void Engine::handleInput() {
    m_input.handleInput();
}