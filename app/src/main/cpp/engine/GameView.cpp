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

void GameView::update(double deltaTime) {

    auto& actor = m_entities[0];
    static float t = 0.0f;
    t+= 0.01;
    auto rot = glm::rotate(glm::mat4(1.0f), (float)t, glm::vec3(0.0f, 1.0f, 0.0f));
    actor->setRotation(glm::vec3(0.0f, 0.0f, t));

    //auto& actor2 = m_entities[1];
    //actor2->setTranslation(glm::vec3(0.0f, 2.0f, 0.0f));

    for (auto& entity : m_entities) {
        entity->update(deltaTime);
    }
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

    /* Test loading assets*/
    auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("debug-uv.png");
    auto texture2 = RESOURCE_MANAGER->get<GPUTextureResource>("android_robot.png");
    auto texture3 = RESOURCE_MANAGER->get<GPUTextureResource>("grass-land-diffuse.jpg");

    auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("cube.osm");
    auto mesh2 = RESOURCE_MANAGER->get<GPUStaticMeshResource>("blender.osm");
    auto tank = RESOURCE_MANAGER->get<GPUStaticMeshResource>("tank.osm");
    auto plane = RESOURCE_MANAGER->get<GPUStaticMeshResource>("plane.osm");

    GPU_RESOURCES->registerMaterial({
        0, 0, 0, 0, 1.0f, 1.0f, 1.0f, 1.0f
    });

    GPU_RESOURCES->registerTexture(*texture->get());

    GPU_RESOURCES->registerTexture(*texture3->get());

    GPU_RESOURCES->registerMaterial({
            1, 1, 1, 1, 1.0f, 1.0f, 1.0f, 1.0f
    });


    auto& actor = m_entities.emplace_back(new OGEntity("actor"));
    auto meshComponent = actor->addComponent<OGStaticMeshComponent>();
    meshComponent->setMeshResource(mesh);
    meshComponent->setTextureResource(TEXTURE_SLOT_0, texture);


    auto actor2 = actor->addChild<OGEntity>("actor2");
    //auto& actor2 = m_entities.emplace_back(new OGEntity("actor2"));
    auto meshComponent2 = actor2->addComponent<OGStaticMeshComponent>();
    meshComponent2->setMeshResource(mesh2);
    meshComponent->setTextureResource(TEXTURE_SLOT_0, texture);
    actor2->setTranslation(glm::vec3(0.0f, 2.0f, 0.0f));

/*
    auto& actor3 = m_entities.emplace_back(new OGEntity("tank"));
    auto meshComponent3 = actor3->addComponent<OGStaticMeshComponent>();
    meshComponent3->setMeshResource(tank);
    meshComponent3->setTextureResource(TEXTURE_SLOT_0, texture);
    actor3->setScale(glm::vec3(2.0f));

    auto& actor4 = m_entities.emplace_back(new OGEntity("plane"));
    auto meshComponent4 = actor4->addComponent<OGStaticMeshComponent>();
    meshComponent4->setMeshResource(plane);
    meshComponent4->setTextureResource(TEXTURE_SLOT_0, texture);
    meshComponent4->setMaterialIndex(1);*/

    /* Testing Colliders */
    m_colliders.emplace_back(new PlaneVolume(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f));

    raycastCallback = [&](const Ray& ray, RaycastHit& hit) {
        //aout << "Raycast Hit: " << hit.m_position.x << ", " << hit.m_position.y << ", " << hit.m_position.z << std::endl;
        auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("cube.osm");
        auto& newActor = m_entities.emplace_back(new OGEntity("plane23"));
        auto meshComp = newActor->addComponent<OGStaticMeshComponent>();
        meshComp->setMeshResource(mesh);
        meshComp->setMaterialIndex(0);
        newActor->setTranslation(hit.m_position);
    };


    return true;
}

void GameView::destroy() {

}
