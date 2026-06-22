//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#include "Input.hpp"
#include "Engine.hpp"
#include "GameView.hpp"
#include "../render/vulkan/Swapchain.hpp"

void Input::handleInput() {
    // Handle input events here
    auto *input = android_app_swap_input_buffers(ENGINE->getApp());
    if (!input) {
        return;
    }

    for (auto i = 0; i < input->motionEventsCount; i++) {
        auto motionEvent = input->motionEvents[i];
        auto action = motionEvent.action;

        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        auto pointer = motionEvent.pointers[pointerIndex];
        auto x = static_cast<float>(SWAPCHAIN->getExtent().width) - GameActivityPointerAxes_getX(&pointer);
        auto y = static_cast<float>(SWAPCHAIN->getExtent().height) - GameActivityPointerAxes_getY(&pointer);

        //auto renderEntities = GAME_VIEW->getEntities()[0];
        //renderEntities->setTranslation(worldPosFar);

        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                break;
            case AMOTION_EVENT_ACTION_UP:{
                glm::vec3 screenPos = glm::vec3(x, y, 1.0f);
                glm::vec3 screenPosNear = glm::vec3(x, y, 0.0f);
                glm::mat4 view = glm::lookAt(glm::vec3(8.0f, 8.0f, 8.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                const float aspect = static_cast<float>(SWAPCHAIN->getExtent().width) / static_cast<float>(SWAPCHAIN->getExtent().height);
                glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.0f);

                glm::vec3  worldPos = glm::unProject(screenPosNear, view, projection, glm::vec4(0.0f, 0.0f, SWAPCHAIN->getExtent().width, SWAPCHAIN->getExtent().height));
                glm::vec3  worldPosFar = glm::unProject(screenPos, view, projection, glm::vec4(0.0f, 0.0f, SWAPCHAIN->getExtent().width, SWAPCHAIN->getExtent().height));

                Ray ray(worldPos, glm::normalize(worldPosFar - worldPos ));

                for(auto &collider : GAME_VIEW->getColliders()) {
                    RaycastHit hit;
                    if(collider->intersect(ray, hit)) {
                        GAME_VIEW->raycastCallback(ray, hit);
                    }
                }

            }
            case AMOTION_EVENT_ACTION_POINTER_UP:
                break;
            case AMOTION_EVENT_ACTION_MOVE:{
                /*for (auto i = 0; i < motionEvent.pointerCount; i++) {
                    pointer = motionEvent.pointers[i];
                    x = GameActivityPointerAxes_getX(&pointer);
                    y = GameActivityPointerAxes_getY(&pointer);
                }*/
            }
                break;
        }
    }

    android_app_clear_motion_events(input);

    android_app_clear_key_events(input);
}
