//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#include "Input.hpp"
#include "../Engine.hpp"
#include "../GameView.hpp"
#include "../../render/vulkan/Swapchain.hpp"
#include "../ui/OGUi.hpp"
#include "../../system/OGTimer.hpp"

void Input::handleInput() {
    auto *input = android_app_swap_input_buffers(ENGINE->getApp());
    if (!input) return;

    float screenWidth = static_cast<float>(SWAPCHAIN->getExtent().width);
    float screenHeight = static_cast<float>(SWAPCHAIN->getExtent().height);
    float scale = DESIGN_HEIGHT / screenHeight;

    for (int i = 0; i < input->motionEventsCount; i++) {
        auto &motionEvent = input->motionEvents[i];
        int32_t action = motionEvent.action;
        int32_t actionCode = action & AMOTION_EVENT_ACTION_MASK;
        int pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        TouchEvent event;
        event.counter = motionEvent.pointerCount;

        for (uint32_t j = 0; j < motionEvent.pointerCount; j++) {
            auto &pointer = motionEvent.pointers[j];
            // Scale raw pixels to design space
            event.touchPoints[j].position = glm::vec2(
                    (GameActivityPointerAxes_getX(&pointer)) * scale,
                    (GameActivityPointerAxes_getY(&pointer)) * scale
            );
            event.touchPoints[j].identifier = pointer.id; // Unique persistent ID
            event.touchPoints[j].isChanged = (j == (uint32_t)pointerIndex);
            event.touchPoints[j].time = SYS_TIMER->getTime(); // Record the time of the event
        }

        switch (actionCode) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                event.eType = TouchEvent::started;
                break;
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                event.eType = TouchEvent::stopped;
                break;
            case AMOTION_EVENT_ACTION_MOVE:
                event.eType = TouchEvent::moved;
                break;
            default: continue;
        }
        processEvents(event);
    }

    android_app_clear_motion_events(input);
}

Input::Input() {

}

void Input::processEvents(TouchEvent &event) {
    float screenWidth = static_cast<float>(SWAPCHAIN->getExtent().width);
    float screenHeight = static_cast<float>(SWAPCHAIN->getExtent().height);
    float scale = DESIGN_HEIGHT / screenHeight;
    float midX = (screenWidth * scale) / 2.0f;

    for (int i = 0; i < event.counter; i++) {
        auto &point = event.touchPoints[i];

        if (event.eType == TouchEvent::started && point.isChanged) {

            event.touchPoints[i].time = SYS_TIMER->getTime(); // Record the time of touch down

            // Assign stick based on which side of the screen was touched
            for (auto &stick : m_thumbSticks) {
                if (!stick->isPressed()) {
                    bool isLeftRegion = (point.position.x < midX);
                    if ((isLeftRegion && stick->getType() == THUMBSTICK_LEFT) ||
                        (!isLeftRegion && stick->getType() == THUMBSTICK_RIGHT)) {
                        stick->onTouchDown(point.position);
                        m_thumbStickMap[point.identifier] = stick; // Map pointer ID to stick
                        break;
                    }
                }
            }
        } else if (event.eType == TouchEvent::moved) {
            // Update movement for any tracked finger
            auto it = m_thumbStickMap.find(point.identifier);
            if (it != m_thumbStickMap.end()) {
                it->second->onTouchMove(point.position);
            }
        } else if (event.eType == TouchEvent::stopped && point.isChanged) {

            double timeDif = SYS_TIMER->getTime() - event.touchPoints[i].time; // Calculate duration of touch
            if (timeDif < 0.8) {

                processTap(event, i);
                event.touchPoints[i].time = SYS_TIMER->getTime();
                //return;
            }

            // Release the stick associated with this specific finger
            auto it = m_thumbStickMap.find(point.identifier);
            if (it != m_thumbStickMap.end()) {
                it->second->onTouchUp(point.position);
                m_thumbStickMap.erase(it);
            }
        }
    }
}

glm::vec2 Input::getPosition() {
    return m_position;
}

std::map<int, ThumbStick*> Input::touchStack(TouchEvent &event) {
    std::map<int, ThumbStick*> output;

    int index, out = 0;
    for (int i = 0; i < event.counter; i++) {
        auto &point = event.touchPoints[i];
        float maxDist = FLT_MAX;
        ThumbStick *closest = nullptr;

        for (auto& stick: m_thumbSticks) {
            float dist = stick->getDistance(event.touchPoints[i].position);
            if (dist < maxDist) {
                maxDist = dist;
                closest = stick.get();
                index = i;
            }
        }
        output[index] = closest;
    }

    return output;
}

bool Input::initialize() {
    addThumbStick(std::make_shared<ThumbStick>(ThumbStick(THUMBSTICK_LEFT, 256.0f, 96.0f)));
    addThumbStick(std::make_shared<ThumbStick>(ThumbStick(THUMBSTICK_RIGHT, 256.0f, 96.0f)));

    for(const auto& stick : m_thumbSticks) {
        stick->initialize();
    }
    return true;
}

void Input::update(double delta) {
    for (const auto& stick : m_thumbSticks) {
        stick->update(delta);
    }
}

void Input::processTap(TouchEvent &event, uint32_t i) {
    const auto& touchPos = event.touchPoints[i].position;

    glm::vec3 screenPos = glm::vec3(touchPos.x, touchPos.y, 1.0f);
    glm::vec3 screenPosNear = glm::vec3(touchPos.x, touchPos.y, 0.0f);
    glm::mat4 view = ENGINE->getCameraView();
    const float aspect = static_cast<float>(SWAPCHAIN->getExtent().width) /
                         static_cast<float>(SWAPCHAIN->getExtent().height);
    glm::mat4 projection = ENGINE->getCameraProjection();

    glm::vec3 worldPos = glm::unProject(screenPosNear, view, projection,
                                        glm::vec4(0.0f, 0.0f,
                                                  SWAPCHAIN->getExtent().width,
                                                  SWAPCHAIN->getExtent().height));
    glm::vec3 worldPosFar = glm::unProject(screenPos, view, projection,
                                           glm::vec4(0.0f, 0.0f,
                                                     SWAPCHAIN->getExtent().width,
                                                     SWAPCHAIN->getExtent().height));

    Ray ray(worldPos, glm::normalize(worldPosFar - worldPos));

    for (auto &collider: GAME_VIEW->getColliders()) {
        RaycastHit hit;
        if (collider->intersect(ray, hit)) {
            GAME_VIEW->raycastCallback(ray, hit);
        }
    }
}
