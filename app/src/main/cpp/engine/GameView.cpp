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
#include "components/OGCollisionComponent.hpp"
#include "engine/ai/AIPathFinding.hpp"
#include "engine/ai/NavMesh.hpp"
#include "IOHelper.hpp"
#include "render/vulkan/pipelines/UIRender.hpp"
#include "engine/actors/OGPlayerActor.hpp"
#include "render/vulkan/Swapchain.hpp"
#include "engine/collision/CollisionFactory.hpp"

void GameView::render() {

}

void GameView::update(double deltaTime) {
    for (auto &entity: m_entities) {
        entity.second->update(deltaTime);
    }
    for (auto &uiElement: UI->getElements()) {
        uiElement->update(deltaTime);
    }

    auto colliders = getActorsWithComponent<OGCollisionComponent>();
    for (auto &c: colliders) {
        c->update(deltaTime);
    }
}

bool GameView::initialize() {

    GPUTexture cubeMapTexture;

    /* CubeMap */
    if (!RenderFramework::createCubMap({
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

    const auto &userInterface = ENGINE->createPipeline<UIRender>("user-interface");

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

    if (!UI->initializeUI()) {
        aout << "Failed to initialize UI" << std::endl;
        return false;
    }

    if (!UI->loadSpriteAsset("", "ui_sprites")) {
        return false;
    }

    /* Test loading assets*/
    auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("blender.osm");

    /* Prepare Game Logic*/

    ENGINE->setCameraPosition(glm::vec3(1.0, 1.0, 1.0));

    if (!loadSceneFile("level1/scene_graph.xml")) {
        aout << "Error: Failed to load scene graph!" << std::endl;
        return false;
    }

    if (!RESOURCE_MANAGER->loadSceneCollision("world.osc", m_worldPolygons)) {
        aout << "Error: Failed to load scene collision!" << std::endl;
        return false;
    }

    NavMesh navMesh(m_worldPolygons);

    auto movableActor = addActor<OGActor>("movableActor");
    auto movableMesh = movableActor->addComponent<OGStaticMeshComponent>();
    movableMesh->setMeshResource(mesh);
    movableMesh->setMaterialIndex(0);
    movableActor->setTranslation(glm::vec3(0.0f, 2.0f, 0.0f));

    raycastCallback = [movableActor, navMesh](const Ray &ray, OGContact &hit) {
        auto path = navMesh.findPath(movableActor->getTranslation(), hit.hitPoint);
        if (!path.empty()) {
            movableActor->setPath(path);
        }
    };

    /* Create UI Elements Button etc*/
    UI->addButton(new OGButton("button1", "sm-button", glm::vec2(100, 100),
                               glm::vec2(128 * 2.5, 32 * 2.5), []() {
                aout << "Button 1 clicked!" << std::endl;
                ENGINE->setGameModeFly(!ENGINE->isGameModeFly());
            }));

    auto spawnPlayer = addActor<OGPlayerActor>("main-player");
    if (spawnPlayer) {
        spawnPlayer->initialize();
        spawnPlayer->setTranslation(glm::vec3(0.0f, 0.0f, 0.0f));

        auto playerMeshComp = spawnPlayer->addComponent<OGStaticMeshComponent>();
        if (playerMeshComp) {
            auto playerMeshRes = RESOURCE_MANAGER->get<GPUStaticMeshResource>("player/player-mesh.osm");
            auto playerTextureAlbedo = RESOURCE_MANAGER->get<GPUTextureResource>("player/textures/player-demo.png");
            auto playerTextureNormal = RESOURCE_MANAGER->get<GPUTextureResource>("player/textures/player-nm.png");

            GPUMaterialHandle playerMaterial = {0, 0, 0, 0, 1.0f, 1.0f, 1.0f, 1.0f};
            playerMaterial.albedoIndex = GPU_RESOURCES->registerTexture(*playerTextureAlbedo->get());
            playerMaterial.normalIndex = GPU_RESOURCES->registerTexture(*playerTextureNormal->get());

            uint32_t materialSlot = GPU_RESOURCES->registerMaterial(playerMaterial);
            playerMeshComp->setMeshResource(playerMeshRes);
            playerMeshComp->setMaterialIndex(materialSlot);

            spawnPlayer->setProjectionMatrix(glm::perspective(glm::radians(60.0f), (float) SWAPCHAIN->getExtent().width / (float)  SWAPCHAIN->getExtent().width, 0.1f, 10000.0f));

            setActivePlayer(spawnPlayer);
        }

        auto collision = spawnPlayer->addComponent<OGCollisionComponent>();
        collision->setVolume(std::unique_ptr<CapsuleVolume>(CollisionFactory::createCapsule(0.5f, 2.0f)));
    }

    return true;
}

bool GameView::loadSceneFile(const std::string &sceneFile) {

    std::vector<std::unique_ptr<OGXmlNode>> sceneGraph;
    if (!OGXml::loadGXml(sceneFile, sceneGraph)) {
        return false;
    }

    std::string scenePath = IOHelper::getFilePath(sceneFile);

    std::map<std::string, GPUMaterialHandle> materials;
    std::map<std::string, uint32_t> materialSlots;
    std::shared_ptr<OGXmlNode> sceneRoot;

    for (const auto &node: sceneGraph) {
        if (node->getName() == "Scene") {
            sceneRoot = std::shared_ptr<OGXmlNode>(node.get(), [](OGXmlNode *) {});
            break;
        }
    }

    if (!sceneRoot) {
        return false;
    }

    for (const auto &node: sceneRoot->getChildren()) {
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
                auto texture = RESOURCE_MANAGER->get<GPUTextureResource>(
                        scenePath + "/textures/" + albedoAttr->second);
                material.albedoIndex = GPU_RESOURCES->registerTexture(*texture->get());
            }

            auto normalAttr = attrs.find("normal");
            if (normalAttr != attrs.end()) {
                auto texture = RESOURCE_MANAGER->get<GPUTextureResource>(
                        scenePath + "/textures/" + normalAttr->second);
                material.normalIndex = GPU_RESOURCES->registerTexture(*texture->get());
            }

            auto pbrAttr = attrs.find("pbr");
            if (pbrAttr != attrs.end()) {
                auto texture = RESOURCE_MANAGER->get<GPUTextureResource>(
                        scenePath + "/textures/" + pbrAttr->second);
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
                    for (const auto &attr: childElem->getAttributes()) {
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
                    for (const auto &attr: childElem->getAttributes()) {
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

                if (childElem->getName() == "Scale") {
                    glm::vec3 scale(1.0f);
                    for (const auto &attr: childElem->getAttributes()) {
                        if (attr.first == "x") {
                            scale.x = std::stof(attr.second);
                        } else if (attr.first == "y") {
                            scale.y = std::stof(attr.second);
                        } else if (attr.first == "z") {
                            scale.z = std::stof(attr.second);
                        }
                    }
                    actorScene->setScale(scale);
                }

                if (childElem->getName() == "MeshResource") {
                    auto valueAttr = childElem->getAttributes().find("value");
                    if (valueAttr != childElem->getAttributes().end()) {
                        auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>(
                                scenePath + "/" + valueAttr->second + ".osm");
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

                if (childElem->getName() == "BoundingBox") {
                    auto minAttr = childElem->getAttributes().find("min");
                    auto maxAttr = childElem->getAttributes().find("max");
                    if (minAttr != childElem->getAttributes().end() &&
                        maxAttr != childElem->getAttributes().end()) {
                        glm::vec3 minVec(0.0f);
                        glm::vec3 maxVec(0.0f);

                        std::istringstream minStream(minAttr->second);
                        minStream >> minVec.x >> minVec.y >> minVec.z;

                        std::istringstream maxStream(maxAttr->second);
                        maxStream >> maxVec.x >> maxVec.y >> maxVec.z;

                        auto AabbVolume = actorScene->addComponent<OGCollisionComponent>();
                        AabbVolume->setVolume(std::make_unique<AABBVolume>(minVec, maxVec));
                    }
                }
            }
        }
    }
    return true;
}

void GameView::destroy() {

}
