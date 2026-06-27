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
#include "algorithms/AStar.hpp"
#include "actors/OGActor.hpp"
#include "../render/vulkan/pipelines/ScreenSpace.hpp"
#include "elements/OGElement.hpp"
#include "ui/OGUi.hpp"

void GameView::render() {

}

void GameView::update(double deltaTime) {
    for (auto &entity: m_entities) {
        entity->update(deltaTime);
    }
}

bool GameView::initialize() {

    /* Prepare Render Pipelines */
    const auto &deferred = ENGINE->createPipeline<Deferred>("deferred");

    const auto &postProcess = ENGINE->createPipeline<PostProcess>("post-process");

    const auto &screenSpace = ENGINE->createPipeline<ScreenSpace>("screen-space");

    /* Set input textures for post-process from deferred G-Buffers */
    postProcess->setFrameBufferImage("gDiffuse", *deferred->getFrameBufferImage("gDiffuse"));
    postProcess->setFrameBufferImage("gNormal", *deferred->getFrameBufferImage("gNormal"));
    postProcess->setFrameBufferImage("gPBR", *deferred->getFrameBufferImage("gPBR"));
    postProcess->setFrameBufferImage("gWorldPosition",
                                     *deferred->getFrameBufferImage("gWorldPosition"));
    postProcess->updateDescriptors();

    /* Test loading assets*/
    auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("debug-uv.png");
    auto texture2 = RESOURCE_MANAGER->get<GPUTextureResource>("android_robot.png");
    auto texture3 = RESOURCE_MANAGER->get<GPUTextureResource>("grass-land-diffuse.jpg");
    auto normal = RESOURCE_MANAGER->get<GPUTextureResource>("grunge1_nm.png");

    auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("tree.osm");
    auto mesh2 = RESOURCE_MANAGER->get<GPUStaticMeshResource>("blender.osm");
    auto tank = RESOURCE_MANAGER->get<GPUStaticMeshResource>("tank.osm");
    auto plane = RESOURCE_MANAGER->get<GPUStaticMeshResource>("plane.osm");

    GPU_RESOURCES->registerMaterial({
                                            0, 1, 1, 0, 1.0f, 1.0f, 1.0f, 1.0f
                                    });

    GPU_RESOURCES->registerTexture(*texture->get());
    GPU_RESOURCES->registerTexture(*normal->get());

    GPU_RESOURCES->registerTexture(*texture3->get());

    GPU_RESOURCES->registerMaterial({
                                            2, 1, 1, 1, 1.0f, 1.0f, 1.0f, 1.0f
                                    });

    /* Prepare UI */
    auto rect = new OGRect();
    rect->create(glm::vec2(0.0f, 0.0f), glm::vec2(100.0f, 100.0f));
    UI->addElement(rect);

    /* Prepare Game Logic*/

    auto &actor = m_entities.emplace_back(new OGEntity("actor"));
    auto meshComponent = actor->addComponent<OGStaticMeshComponent>();
    meshComponent->setMeshResource(mesh);
    meshComponent->setTextureResource(TEXTURE_SLOT_0, texture);
    meshComponent->setMaterialIndex(1);

    auto &actor4 = m_entities.emplace_back(new OGEntity("plane"));
    auto meshComponent4 = actor4->addComponent<OGStaticMeshComponent>();
    meshComponent4->setMeshResource(plane);
    meshComponent4->setTextureResource(TEXTURE_SLOT_0, texture);
    meshComponent4->setMaterialIndex(1);

    auto playerActor = addActor<OGActor>("player-actor");
    auto playerMesh = playerActor->addComponent<OGStaticMeshComponent>();
    playerMesh->setMeshResource(tank);
    playerMesh->setTextureResource(TEXTURE_SLOT_0, texture2);
    playerMesh->setMaterialIndex(0);
    playerActor->setTranslation(glm::vec3(0.0f, 2.0f, 0.0f));


    /* Testing Colliders */
    m_colliders.emplace_back(new PlaneVolume(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f));

    Grid path2D(100, std::vector<int>(100, 0));

    raycastCallback = [playerActor, path2D](const Ray &ray, RaycastHit &hit) {

        Path path;
        auto endPoint = AStar::worldToGrid(hit.m_position, 1.5f);
        std::vector<glm::vec3> pathPoints;
        if (AStar::execute(path2D, AStar::worldToGrid(playerActor->getTranslation(), 1.5f),
                           endPoint, path)) {
            for (auto &point: path) {
                pathPoints.push_back(AStar::gridToWorld(point, 1.5f));
            }
            playerActor->setPath(pathPoints);
        } else {
            playerActor->setPath(std::vector<glm::vec3>());
        }
    };


    return true;
}

void GameView::destroy() {

}
