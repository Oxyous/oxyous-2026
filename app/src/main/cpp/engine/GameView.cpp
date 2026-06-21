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
    auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("android_robot.png");

    auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("cube.osm");
    auto mesh2 = RESOURCE_MANAGER->get<GPUStaticMeshResource>("cube.osm");

    auto& actor = m_entities.emplace_back(new OGEntity("actor"));
    actor->addComponent<OGStaticMeshComponent>();
    actor->getComponent<OGStaticMeshComponent>()->setMeshResource(mesh);
    actor->getComponent<OGStaticMeshComponent>()->setTextureResource(TEXTURE_SLOT_0, texture);



    return true;
}

void GameView::destroy() {

}
