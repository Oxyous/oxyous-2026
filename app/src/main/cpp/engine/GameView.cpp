//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#include "GameView.hpp"
#include "../resources/ResourceManager.hpp"
#include "../DataStructures.hpp"
#include "../resources/GPUTextureResource.hpp"
#include "../render/meshes/GPUStaticMesh.hpp"

void GameView::render() {

}

void GameView::update(float deltaTime) {

}

bool GameView::initialize() {

    /* Test loading assets*/
    auto texture = RESOURCE_MANAGER->get<GPUTextureResource>("android_robot.png");

    auto mesh = RESOURCE_MANAGER->get<GPUStaticMeshResource>("cube.osm");

    return true;
}

void GameView::destroy() {

}
