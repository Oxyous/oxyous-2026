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
#include "engine/actors/OGCharacter.hpp"
#include "engine/components/OGPhysicsComponent.hpp"
#include "engine/physics/OGPhysicsManager.hpp"
#include "system/OGTimer.hpp"

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

    //ENGINE->setCameraPosition(glm::vec3(1.0, 1.0, 1.0));

    if (!loadSceneFile("level1/scene_graph.xml")) {
        aout << "Error: Failed to load scene graph!" << std::endl;
        return false;
    }

    if (!RESOURCE_MANAGER->loadSceneCollision("world.osc", m_worldPolygons)) {
        aout << "Error: Failed to load scene collision!" << std::endl;
        return false;
    }

    /** Box Resources */
    auto boxMeshRes = RESOURCE_MANAGER->get<GPUStaticMeshResource>("box/box.osm");
    auto boxAlbedo = RESOURCE_MANAGER->get<GPUTextureResource>("box/textures/box-diffuse.png");
    auto boxNormal = RESOURCE_MANAGER->get<GPUTextureResource>("box/textures/box-normal.jpg");

    GPUMaterialHandle boxMaterial = {0, 0, 0, 0, 1.0f, 1.0f, 1.0f, 1.0f};
    boxMaterial.albedoIndex = GPU_RESOURCES->registerTexture(*boxAlbedo->get());
    boxMaterial.normalIndex = GPU_RESOURCES->registerTexture(*boxNormal->get());
    uint32_t boxMaterialSlot = GPU_RESOURCES->registerMaterial(boxMaterial);

    /** AI Test */
    NavMesh navMesh(m_worldPolygons);

    auto movableActor = addActor<OGCharacter>("movableActor");
    auto movableMesh = movableActor->addComponent<OGStaticMeshComponent>();
    movableMesh->setMeshResource(mesh);
    movableMesh->setMaterialIndex(0);
    movableActor->setTranslation(glm::vec3(0.0f, 2.0f, 0.0f));

    raycastCallback = [&](const Ray &ray, OGContact &hit) {
        /*auto path = navMesh.findPath(movableActor->getTranslation(), hit.hitPoint);
        if (!path.empty()) {
            movableActor->setPath(path);
        }*/

        /** Box Resources */
        /*
        auto boxMeshRes = RESOURCE_MANAGER->get<GPUStaticMeshResource>("box/box.osm");
        auto boxAlbedo = RESOURCE_MANAGER->get<GPUTextureResource>("box/textures/box-diffuse.png");
        auto boxNormal = RESOURCE_MANAGER->get<GPUTextureResource>("box/textures/box-normal.jpg");

        GPUMaterialHandle boxMaterial = {3, 3, 0, 0, 1.0f, 1.0f, 1.0f, 1.0f};
        //boxMaterial.albedoIndex = GPU_RESOURCES->registerTexture(*boxAlbedo->get());
        //boxMaterial.normalIndex = GPU_RESOURCES->registerTexture(*boxNormal->get());
        uint32_t boxMaterialSlot = GPU_RESOURCES->registerMaterial(boxMaterial);

        glm::vec3 spawnPos = glm::vec3(hit.hitPoint.x, 20.0f, hit.hitPoint.z);

        auto boxSpawn = addActor<OGActor>("box-" + std::to_string(m_entities.size()+1));
        boxSpawn->setTranslation(glm::vec3(spawnPos));
        boxSpawn->setRotation(glm::vec3(glm::radians(15.0f), glm::radians(15.0f), glm::radians(15.0f)));
        auto boxMesh = boxSpawn->addComponent<OGStaticMeshComponent>();
        boxMesh->setMeshResource(boxMeshRes);
        boxMesh->setMaterialIndex(boxMaterialSlot);
        auto boxPhys = boxSpawn->addComponent<OGPhysicsComponent>();
        auto boxObb = boxSpawn->addComponent<OGCollisionComponent>();
        auto boxBound = CollisionFactory::createOBB(glm::vec3(0.0,0.0,0.0), glm::vec3(0.5f, 0.5f, 0.5f), glm::mat3(1.0f));
        boxObb->setVolume(std::unique_ptr<OBBVolume>(boxBound));
        boxPhys->setMass(1.0f);
        PHYSICS->registerPhysicsActor(boxSpawn);*/

    };

    /** Create UI Elements Button etc*/
    UI->addButton(new OGButton("button1", "sm-button", glm::vec2(100, 100),
                               glm::vec2(128 * 2.5, 32 * 2.5), []() {
                aout << "Button 1 clicked!" << std::endl;
                ENGINE->setGameModeFly(!ENGINE->isGameModeFly());
            }));

    /** Create UI Elements Button etc*/
    UI->addButton(new OGButton("button2", "sm-button", glm::vec2(100, 174),
                               glm::vec2(128 * 2.5, 32 * 2.5), [&]() {
                aout << "Button 1 clicked!" << std::endl;
            }));

    /** Create Player Character */
    auto spawnPlayer = addActor<OGPlayerActor>("main-player");
    if (spawnPlayer) {
        spawnPlayer->initialize();
        spawnPlayer->setTranslation(glm::vec3(-2.0f, 0.0f, 2.0f));


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

            auto collision = spawnPlayer->addComponent<OGCollisionComponent>();

            //collision->setVolume(std::unique_ptr<OBBVolume>(CollisionFactory::createOBB(glm::vec3(0.0,0.0,0.0), glm::vec3(0.25f,1.0f,0.25f), glm::mat3(1.0f))));
            collision->setVolume(std::unique_ptr<CapsuleVolume>(CollisionFactory::createCapsule(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,1.0f,0.0f), 0.25f)));

            auto playerPhys = spawnPlayer->addComponent<OGPhysicsComponent>();
            playerPhys->setMass(0.0f);
            playerPhys->setVelocity(glm::vec3(0.0f, 0.0f, 0.0f));
            playerPhys->setAngularVelocity(glm::vec3(0.0f, 0.0f, 0.0f));
            playerPhys->setAcceleration(glm::vec3(0.0f, 0.0f, 0.0f));

            setActivePlayer(spawnPlayer);
            PHYSICS->registerPhysicsActor(spawnPlayer);
        }
    }

    /** Physics Test - Ground*/
    auto ground = addActor<OGActor>("ground-plane");
    auto groundPhys = ground->addComponent<OGPhysicsComponent>();
    auto groundObb = ground->addComponent<OGCollisionComponent>();
    groundObb->setVolume(std::unique_ptr<OBBVolume>(CollisionFactory::createOBB(glm::vec3(0.0f,0.0f,0.0f), glm::vec3(50.0f, 1.0f, 50.0f), glm::mat3(1.0f))));
    ground->setTranslation(glm::vec3(0.0f, -1.0f, 0.0f));
    groundPhys->setMass(0.0f);

    /** Physics test - Box */

    for(int i = 0; i < 5; i++) {
        auto box = addActor<OGActor>("box-" + std::to_string(m_entities.size()+1));
        box->setTranslation(glm::vec3(0.0f, (3.0f * (float)i) + 50.0f, 0.0f));
        box->setRotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), glm::radians(45.0f)));
        auto boxMesh = box->addComponent<OGStaticMeshComponent>();
        boxMesh->setMeshResource(boxMeshRes);
        boxMesh->setMaterialIndex(boxMaterialSlot);
        auto boxPhys = box->addComponent<OGPhysicsComponent>();
        auto boxObb = box->addComponent<OGCollisionComponent>();
        auto boxBound = CollisionFactory::createOBB(glm::vec3(0.0,0.0,0.0), glm::vec3(0.5f,0.5f,0.5f), glm::mat3(1.0f));
        boxObb->setVolume(std::unique_ptr<OBBVolume>(boxBound));
        boxPhys->setMass(1.0f);
        PHYSICS->registerPhysicsActor(box);
    }

    auto camera = addActor<OGCamera>("camera");
    auto sphere = camera->addComponent<OGCollisionComponent>();
    sphere->setVolume(std::unique_ptr<SphereVolume>(CollisionFactory::createSphere(glm::vec3(0.0f, 0.0f, 0.0f), 0.5f)));
    auto camPhys = camera->addComponent<OGPhysicsComponent>();
    camPhys->setAcceleration(glm::vec3(0.0f, 0.0f, 0.0f));
    camPhys->setMass(0.0f);
    camera->setTranslation(glm::vec3(0.0f, 5.0f, 10.0f));

    ENGINE->setCamera(camera);

    PHYSICS->registerPhysicsActor(ground);
    PHYSICS->registerPhysicsActor(camera);
    SYS_TIMER->Start();
    PHYSICS->start();

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
