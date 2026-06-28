# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("oxyous_2026")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(oxyous_2026 SHARED
        main.cpp
        engine/algorithms/AStar.cpp
        engine/actors/OGActor.cpp
        engine/collision/PlaneVolume.cpp
        engine/collision/AABBVolume.cpp
        engine/collision/BHV.cpp
        engine/input/Input.cpp
        engine/input/ThumbStick.cpp
        engine/elements/OGElement.cpp
        engine/elements/ScreenSpaceRenderer.cpp
        engine/GameView.cpp
        engine/Engine.cpp
        engine/GPUResources.cpp
        system/OGTimer.cpp
        render/vulkan/pipelines/ScreenSpace.cpp
        render/meshes/GPUStaticMesh.cpp
        render/vulkan/pipelines/PostProcess.cpp
        render/vulkan/pipelines/Deferred.cpp
        render/vulkan/Renderer.cpp
        render/vulkan/Swapchain.cpp
        render/vulkan/RenderDevice.cpp
        resources/ResourceManager.cpp
        resources/GPUTextureResource.cpp
        engine/components/OGStaticMeshComponent.cpp
        AndroidOut.cpp
)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Forces the linker to keep the JNI entry point for GameActivity
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

set(CMAKE_ANDROID_API 30)

# Configure libraries CMake uses to link your target library.
target_link_libraries(oxyous_2026
        # The game activity
        game-activity::game-activity_static

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        vulkan
        jnigraphics
        android
        log)