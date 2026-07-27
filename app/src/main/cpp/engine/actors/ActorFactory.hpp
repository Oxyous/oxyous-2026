//
// Created by Mr Steven J Baldwin on 12/07/2026.
//

#ifndef OXYOUS_2026_ACTORFACTORY_HPP
#define OXYOUS_2026_ACTORFACTORY_HPP

#include <memory>
#include "../entity/OGEntity.hpp"
#include "OGPlayerActor.hpp"
#include "OGCharacter.hpp"
#include "engine/components/OGSkeletalMeshComponent.hpp"
#include "engine/GPUResources.hpp"
#include "render/vulkan/Swapchain.hpp"
#include "engine/components/OGCollisionComponent.hpp"
#include "engine/collision/CollisionFactory.hpp"
#include "engine/physics/OGPhysicsManager.hpp"
#include "engine/Engine.hpp"

class ActorFactory {
public:
    template<typename... Args>
    static std::unique_ptr<OGPlayerActor> createPlayerActor(Args&&... args) {
        auto actor = createActor<OGPlayerActor>(std::forward<Args>(args)...);

        auto meshComp = actor->template addComponent<OGSkeletalMeshComponent>();
        if (meshComp) {
            auto skeletalMesh = RESOURCE_MANAGER->get<GPUSkeletalMeshResource>("animations/player/player.gmesh");
            if (skeletalMesh) {
                meshComp->setMeshResource(skeletalMesh);
            }

            auto diffuse = RESOURCE_MANAGER->get<GPUTextureResource>("textures/player-diffuse.png");
            auto normal = RESOURCE_MANAGER->get<GPUTextureResource>("textures/player-nm.png");


            GPUMaterialHandle material = {
                .albedoIndex = GPU_RESOURCES->registerTexture(*diffuse->get()),
                .normalIndex = GPU_RESOURCES->registerTexture(*normal->get()),
            };
            meshComp->setMaterialIndex(GPU_RESOURCES->registerMaterial(material));
        }

        actor->setName("player");
        actor->setTranslation(glm::vec3(-2.0f, 0.0f, 2.0f));

        actor->setProjectionMatrix(glm::perspective(glm::radians(60.0f),
                                                    (float) SWAPCHAIN->getExtent().width /
                                                    (float) SWAPCHAIN->getExtent().width,
                                                    0.1f, 10000.0f));

        auto collision = actor->template addComponent<OGCollisionComponent>();
        collision->setVolume(std::unique_ptr<CapsuleVolume>(
                CollisionFactory::createCapsule(glm::vec3(0.0f, 0.0f, 0.0f),
                                                glm::vec3(0.0f, 1.0f, 0.0f), 0.25f)));

        auto playerPhys = actor->template addComponent<OGPhysicsComponent>();
        playerPhys->setMass(1.0f);
        playerPhys->setVelocity(glm::vec3(0.0f, 0.0f, 0.0f));
        playerPhys->setAngularVelocity(glm::vec3(0.0f, 0.0f, 0.0f));
        playerPhys->setAcceleration(glm::vec3(0.0f, 0.0f, 0.0f));
        playerPhys->setRotationLock(true, true, true);
        playerPhys->setRestitution(0.0f);

        GAME_VIEW->setActivePlayer(actor.get());
        PHYSICS->registerPhysicsActor(actor.get());

        return actor;
    }


    /** Create NPC Actor*/
    template<typename... Args>
    static std::unique_ptr<OGCharacter> createNPC(Args&&... args) {
        auto actor = createActor<OGCharacter>(std::forward<Args>(args)...);
        actor->setName("NPC");
        actor->setTranslation(glm::vec3(-3.0f, 0.0f, 2.0f));
        auto skeletalMesh = actor->template addComponent<OGSkeletalMeshComponent>();
        if (skeletalMesh) {
            auto meshResource = RESOURCE_MANAGER->get<GPUSkeletalMeshResource>("animations/player/player.gmesh");

            if (meshResource) {
                skeletalMesh->setMeshResource(meshResource);
            }

            auto diffuse = RESOURCE_MANAGER->get<GPUTextureResource>("textures/player-diffuse.png");
            auto normal = RESOURCE_MANAGER->get<GPUTextureResource>("textures/player-nm.png");

            GPUMaterialHandle material = {
                .albedoIndex = GPU_RESOURCES->registerTexture(*diffuse->get()),
                .normalIndex = GPU_RESOURCES->registerTexture(*normal->get()),
            };
            skeletalMesh->setMaterialIndex(GPU_RESOURCES->registerMaterial(material));
        }

        return actor;
    }

private:
    template<typename T, typename... Args>
    static std::unique_ptr<T> createActor(Args&&... args) {
        static_assert(std::is_base_of<OGEntity, T>::value, "T must be derived from OGEntity");
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
};


#endif //OXYOUS_2026_ACTORFACTORY_HPP
