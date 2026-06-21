//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "GameView.hpp"
#include "../resources/ResourceManager.hpp"
#include "../DataStructures.hpp"
#include "../resources/GPUTextureResource.hpp"
#include "../render/meshes/GPUStaticMesh.hpp"
#include "../render/vulkan/DescriptorCache.hpp"
#include "Engine.hpp"
#include "../render/vulkan/pipelines/Deferred.hpp"
#include "../render/vulkan/pipelines/PostProcess.hpp"
#include "components/OGStaticMeshComponent.hpp"
#include "GPUResources.hpp"

void GameView::render() {

}

void GameView::update(float deltaTime) {

}

bool GameView::initialize() {

    /* Prepare Render Pipelines */
    const auto &deferred = ENGINE->createPipeline<Deferred>("deferred");

    const auto &postProcess = ENGINE->createPipeline<PostProcess>("post-process");

    /* Set input textures for post-process from deferred G-Buffers */
    postProcess->setFrameBufferImage("gDiffuse", *deferred->getFrameBufferImage("gDiffuse"));
    postProcess->setFrameBufferImage("gNormal", *deferred->getFrameBufferImage("gNormal"));
    postProcess->setFrameBufferImage("gPBR", *deferred->getFrameBufferImage("gPBR"));
    postProcess->setFrameBufferImage("gWorldPosition", *deferred->getFrameBufferImage("gWorldPosition"));
    postProcess->updateDescriptors();

    postProcess->setFrameBufferImage("gDiffuse", *deferred->getFrameBufferImage("gDiffuse"));
    postProcess->setFrameBufferImage("gNormal", *deferred->getFrameBufferImage("gNormal"));
    postProcess->setFrameBufferImage("gPBR", *deferred->getFrameBufferImage("gPBR"));
    postProcess->setFrameBufferImage("gWorldPosition", *deferred->getFrameBufferImage("gWorldPosition"));
    postProcess->setFrameBufferImage("gDepth", *deferred->getFrameBufferImage("gDepth"));

    postProcess->updateDescriptors();

    /* Test loading assets*/
    auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("debug-uv.jpg");

    auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("cube.osm");
    auto mesh2 = RESOURCE_MANAGER->get<GPUStaticMeshResource>("cube.osm");

    GPU_RESOURCES->registerMaterial({
        0, 0, 0, 0, 1.0f, 1.0f, 1.0f, 1.0f
    });

    GPU_RESOURCES->registerTexture(*texture->get());

    auto& actor = m_entities.emplace_back(new OGEntity("actor"));
    auto meshComponent = actor->addComponent<OGStaticMeshComponent>();
    meshComponent->setMeshResource(mesh);
    meshComponent->setTextureResource(TEXTURE_SLOT_0, texture);


    return true;
}

void GameView::destroy() {

}
