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
#include "actors/OGCamera.hpp"
#include "../render/vulkan/pipelines/ShadowCapture.hpp"
#include "../system/OGXml.hpp"

void GameView::render() {

}

void GameView::update(double deltaTime) {
    for (auto &entity: m_entities) {
        entity.second->update(deltaTime);
    }
    for (auto &uiElement : UI->getElements()) {
        uiElement->update(deltaTime);
    }
}

bool GameView::initialize() {

    GPUTexture cubeMapTexture;

    /* CubeMap */
    if(!RenderFramework::createCubMap({
                                              "cubemap/sky_right.png",
                                              "cubemap/sky_left.png",
                                              "cubemap/sky_up.png",
                                              "cubemap/sky_down.png",
                                              "cubemap/sky_forward.png",
                                              "cubemap/sky_back.png"
                                      }, 1024, &cubeMapTexture)) {
        aout << "Error: Failed to create cubemap!" << std::endl;
        return false;
    }

    /* Prepare Render Pipelines */
    const auto &deferred = ENGINE->createPipeline<Deferred>("deferred");

    const auto &postProcess = ENGINE->createPipeline<PostProcess>("post-process");

    const auto &screenSpace = ENGINE->createPipeline<ScreenSpace>("screen-space");

    const auto &shadowPass = ENGINE->createPipeline<ShadowCapture>("shadow-capture");

    /* Set input textures for post-process from deferred G-Buffers */
    postProcess->setFrameBufferImage("gDiffuse", *deferred->getFrameBufferImage("gDiffuse"));
    postProcess->setFrameBufferImage("gNormal", *deferred->getFrameBufferImage("gNormal"));
    postProcess->setFrameBufferImage("gPBR", *deferred->getFrameBufferImage("gPBR"));
    postProcess->setFrameBufferImage("gWorldPosition",
                                     *deferred->getFrameBufferImage("gWorldPosition"));
    postProcess->setFrameBufferImage("gDepth", *deferred->getFrameBufferImage("gDepth"));
    postProcess->setFrameBufferImage("gEnvironment", cubeMapTexture.descriptor);
    postProcess->setFrameBufferImage("gShadow", shadowPass->getFrameBufferImage("gShadow"));
    postProcess->updateDescriptors();

    /* Test loading assets*/
    auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("debug-uv.png");
    auto texture2 = RESOURCE_MANAGER->get<GPUTextureResource>("android_robot.png");
    auto texture3 = RESOURCE_MANAGER->get<GPUTextureResource>("grass-land-diffuse.jpg");
    auto normal = RESOURCE_MANAGER->get<GPUTextureResource>("grunge1_nm.png");
    auto metalPanel = RESOURCE_MANAGER->get<GPUTextureResource>("metal-panel.jpg");

    auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("tree.osm");
    auto mesh2 = RESOURCE_MANAGER->get<GPUStaticMeshResource>("scene/brick-walls.osm");
    auto tank = RESOURCE_MANAGER->get<GPUStaticMeshResource>("tank.osm");
    auto plane = RESOURCE_MANAGER->get<GPUStaticMeshResource>("plane.osm");

    GPU_RESOURCES->registerMaterial({
                                            0, 1, 1, 0, 1.0f, 1.0f, 1.0f, 1.0f
                                    });

    GPU_RESOURCES->registerTexture(*metalPanel->get());
    GPU_RESOURCES->registerTexture(*normal->get());

    GPU_RESOURCES->registerTexture(*texture3->get());

    GPU_RESOURCES->registerMaterial({
                                            2, 1, 1, 1, 1.0f, 1.0f, 1.0f, 1.0f
                                    });

    /* Prepare Game Logic*/

    auto actor = addActor<OGActor>("actor");
    auto meshComponent = actor->addComponent<OGStaticMeshComponent>();
    meshComponent->setMeshResource(mesh);
    meshComponent->setTextureResource(TEXTURE_SLOT_0, texture);
    meshComponent->setMaterialIndex(1);

    auto actor4 = addActor<OGActor>("plane");
    auto meshComponent4 = actor4->addComponent<OGStaticMeshComponent>();
    meshComponent4->setMeshResource(plane);
    meshComponent4->setTextureResource(TEXTURE_SLOT_0, texture);
    meshComponent4->setMaterialIndex(1);

    auto playerActor = addActor<OGActor>("player-actor");
    auto playerMesh = playerActor->addComponent<OGStaticMeshComponent>();
    playerMesh->setMeshResource(tank);
    playerMesh->setTextureResource(TEXTURE_SLOT_0, texture2);
    playerMesh->setMaterialIndex(0);
    playerActor->setTranslation(glm::vec3(0.0f, 0.0f, 0.0f));

    auto child = playerActor->addChild<OGActor>();
    auto testSecondComp = child->addComponent<OGStaticMeshComponent>();
    testSecondComp->setMeshResource(mesh2);
    testSecondComp->setTextureResource(TEXTURE_SLOT_0, texture2);
    testSecondComp->setMaterialIndex(1);
    child->setTranslation(glm::vec3(0.0,2.0,0.0));


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

    std::vector<std::unique_ptr<OGXmlNode>> sceneGraph;
    if (!OGXml::loadGXml("demo/scene_graph.xml", sceneGraph)) {
        return false;
    }
    std::map<std::string, GPUMaterialHandle> materials;
    std::map<std::string, uint32_t> materialSlots;
    std::shared_ptr<OGXmlNode> sceneRoot;

    for (const auto &node : sceneGraph) {
        if (node->getName() == "Scene") {
            sceneRoot = std::shared_ptr<OGXmlNode>(node.get(), [](OGXmlNode *) {});
            break;
        }
    }

    if (!sceneRoot) {
        return false;
    }

    for (const auto &node : sceneRoot->getChildren()) {
        if (node->getName() == "Material") {
            const auto &attrs = node->getAttributes();
            auto nameAttr = attrs.find("name");
            if (nameAttr == attrs.end()) {
                continue;
            }

            std::string materialName = nameAttr->second;
            GPUMaterialHandle material = {0, 0, 0, 0, 1.0f, 1.0f, 1.0f, 1.0f};

            auto albedoAttr = attrs.find("albedo");
            if (albedoAttr != attrs.end()) {
                auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("demo/textures/" + albedoAttr->second);
                material.albedoIndex = GPU_RESOURCES->registerTexture(*texture->get());
            }

            auto normalAttr = attrs.find("normal");
            if (normalAttr != attrs.end()) {
                auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("demo/textures/" + normalAttr->second);
                material.normalIndex = GPU_RESOURCES->registerTexture(*texture->get());
            }

            auto pbrAttr = attrs.find("pbr");
            if (pbrAttr != attrs.end()) {
                auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("demo/textures/" + pbrAttr->second);
                material.ormIndex = GPU_RESOURCES->registerTexture(*texture->get());
            }

            materials[materialName] = material;
            materialSlots[materialName] = GPU_RESOURCES->registerMaterial(material);
        }

        if (node->getName() == "Object") {
            const auto &objectAttrs = node->getAttributes();
            std::string name;
            auto nameAttr = objectAttrs.find("name");
            if (nameAttr != objectAttrs.end()) {
                name = nameAttr->second;
            }

            auto actorScene = addActor<OGActor>(name);
            auto meshComp = actorScene->addComponent<OGStaticMeshComponent>();
            actorScene->setName(name);

            for (const auto &childElem: node->getChildren()) {
                if (childElem->getName() == "Location") {
                    glm::vec3 position(0.0f);
                    for (const auto &attr : childElem->getAttributes()) {
                        if (attr.first == "x") {
                            position.x = std::stof(attr.second);
                        } else if (attr.first == "y") {
                            position.y = std::stof(attr.second);
                        } else if (attr.first == "z") {
                            position.z = std::stof(attr.second);
                        }
                    }
                    actorScene->setTranslation(position);
                }

                if (childElem->getName() == "Rotation") {
                    glm::vec3 q(0.0f);
                    for (const auto &attr : childElem->getAttributes()) {
                        if (attr.first == "x") {
                            q.x = std::stof(attr.second);
                        } else if (attr.first == "y") {
                            q.y = std::stof(attr.second);
                        } else if (attr.first == "z") {
                            q.z = std::stof(attr.second);
                        }
                    }
                    actorScene->setRotation(q);
                }

                if (childElem->getName() == "MeshResource") {
                    auto valueAttr = childElem->getAttributes().find("value");
                    if (valueAttr != childElem->getAttributes().end()) {
                        auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("demo/" + valueAttr->second + ".osm");
                        meshComp->setMeshResource(mesh);
                    }
                }

                if (childElem->getName() == "Material") {
                    auto materialAttr = childElem->getAttributes().find("name");
                    if (materialAttr != childElem->getAttributes().end()) {
                        auto slotIt = materialSlots.find(materialAttr->second);
                        if (slotIt != materialSlots.end()) {
                            meshComp->setMaterialIndex(slotIt->second);
                        }
                    }
                }
            }
        }
    }

    return true;
}

void GameView::destroy() {

}
