#include <jni.h>

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <game-activity/GameActivity.h>
#include <android/native_window.h>

#include "AndroidOut.h"
#include "render/vulkan/Renderer.hpp"
#include "resources/ResourceManager.hpp"
#include "engine/Engine.hpp"
#include "system/OGTimer.hpp"

extern "C" {

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(android_app *pApp, int32_t cmd) {
    auto gameEngine = (struct appEngine *) pApp->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            // A new window is created, associate a renderer with it. You may replace this with a
            // "game" class if that suits your needs. Remember to change all instances of userData
            // if you change the class here as a reinterpret_cast is dangerous this in the
            // android_main function and the APP_CMD_TERM_WINDOW handler case.
            if (!gameEngine || pApp->window == nullptr) {
                aout << "Window is null or gameEngine is null" << std::endl;
                return;
            }

            int32_t width = ANativeWindow_getWidth(pApp->window);
            int32_t height = ANativeWindow_getHeight(pApp->window);

            ANativeWindow_setBuffersGeometry(pApp->window, width, height, WINDOW_FORMAT_RGBA_8888);

            ENGINE->initialize(pApp);

            gameEngine->width = width;
            gameEngine->height = height;

            if (!gameEngine->renderer) {
                gameEngine->renderer = new Renderer();
                gameEngine->renderer->setWidth(width);
                gameEngine->renderer->setHeight(height);
                if (!gameEngine->renderer->initialize(pApp->window)) {
                    aout << "Error: Failed to initialize renderer" << std::endl;
                }
            }else {
                gameEngine->renderer->setWidth(width);
                gameEngine->renderer->setHeight(height);
                gameEngine->renderer->destroy();
                if (!gameEngine->renderer->initialize(pApp->window)) {
                    aout << "Error: Failed to initialize renderer" << std::endl;
                }
            }


        }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being destroyed. Use this to clean up your userData to avoid leaking
            // resources.
            //
            // We have to check if userData is assigned just in case this comes in really quickly
            if (gameEngine && gameEngine->renderer) {
                gameEngine->renderer->destroy();
                delete gameEngine->renderer;
                gameEngine->renderer = nullptr;
            }
            break;
        default:
            break;
    }
}

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer or joystick device,
 *         false for all other input devices.
 */
bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    // Can be removed, useful to ensure your code is running
    aout << "Welcome to android_main" << std::endl;

    struct appEngine gameEngine = {};
    gameEngine.app = pApp;

    // Register an event handler for Android events
    pApp->onAppCmd = handle_cmd;
    pApp->userData = &gameEngine;

    SYS_TIMER->Start();


    RESOURCE_MANAGER->setAssetManager(pApp->activity->assetManager);

    // Set input event filters (set it to NULL if the app wants to process all inputs).
    // Note that for key inputs, this example uses the default default_key_filter()
    // implemented in android_native_app_glue.c.
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    // This sets up a typical game/event loop. It will run until the app is destroyed.
    do {
        // Process all pending events before running game logic.
        bool done = false;
        while (!done) {
            // 0 is non-blocking.
            int timeout = 0;
            int events;
            android_poll_source *pSource;
            int result = ALooper_pollOnce(timeout, nullptr, &events,
                                          reinterpret_cast<void**>(&pSource));
            switch (result) {
                case ALOOPER_POLL_TIMEOUT:
                    [[clang::fallthrough]];
                case ALOOPER_POLL_WAKE:
                    // No events occurred before the timeout or explicit wake. Stop checking for events.
                    done = true;
                    break;
                case ALOOPER_EVENT_ERROR:
                    aout << "ALooper_pollOnce returned an error" << std::endl;
                    break;
                case ALOOPER_POLL_CALLBACK:
                    break;
                default:
                    if (pSource) {
                        pSource->process(pApp, pSource);
                    }
            }
        }

        // Check if any user data is associated. This is assigned in handle_cmd
        if (pApp->userData) {
            auto *gameEngine = reinterpret_cast<appEngine *>(pApp->userData);

            if (gameEngine->renderer) {
                ENGINE->handleInput();
                ENGINE->update(SYS_TIMER->GetDelta());
                SYS_TIMER->Tick();

                // Update then Render
                gameEngine->renderer->update(SYS_TIMER->GetDelta());
                gameEngine->renderer->render();
            }
        }
    } while (!pApp->destroyRequested);
}
}