//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_ENGINE_HPP
#define OXYOUS_2026_ENGINE_HPP

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include "../includes.hpp"
#include "../system/OGSingleton.hpp"
#include "../render/vulkan/pipelines/IRenderPipeline.hpp"
#include "input/Input.hpp"
#include "actors/OGActor.hpp"
#include "actors/OGCamera.hpp"
#include "../DataStructures.hpp"

class Engine {
public:
    Engine() = default;
    virtual ~Engine() = default;
public:
    /* Initialize Game Engine */
    virtual bool initialize(android_app* app);

    /* Destroy Game Engine */
    virtual void destroy();

    /*  Update Game Engine */
    virtual void update(float deltaTime);

    /*  Render Game Engine */
    virtual void render();

    /* handle input */
    virtual void handleInput();

    /* */
    virtual void prepareInput();

    virtual bool postInitialize();

    virtual glm::mat4 preRotation();

    virtual ThumbStick* getThumbStick(ThumbStickType type);
public:
    template<typename T, typename... TArgs>
    T* createPipeline(const std::string& name, TArgs&&... args) {
        T *renderPipeline(new T(std::forward<TArgs>(args)...));

        std::unique_ptr<IRenderPipeline> uPtr{renderPipeline};

        m_pipelines[name] = std::move(uPtr);

        m_pipelines[name]->initialize();

        return (T *) m_pipelines[name].get();
    }

    template<typename T>
    T* getPipeline(const std::string& name)
    {
        auto it = m_pipelines.find(name);
        if (it == m_pipelines.end()) {
            return nullptr;
        }

        return dynamic_cast<T*>(it->second.get());
    }

    /* Get Android App */
    android_app* getApp() {
        return m_app;
    }

    glm::mat4 getCameraView(){
        return m_camera.getViewMatrix();
    }

    glm::mat4 getCameraProjection(){
        return m_camera.getProjectionMatrix();
    }

    glm::vec3 getCameraPosition(){
        return m_camera.getPosition();
    }

    void setCameraProjection(glm::mat4 projection){
        m_camera.setProjectionMatrix(projection);
    }

    void setCameraPosition(glm::vec3 position) {
        m_camera.setTranslation(position);
    }

    void setCameraRotation(glm::vec3 rotation) {
        m_camera.setRotation(rotation);
    }

    void setSharedCSMData(const CSMData& data) { m_sharedCSMData = data; }
    CSMData getSharedCSMData() const { return m_sharedCSMData; }

    void setCurrentFrame(uint32_t frame) { m_currentFrame = frame; }
    uint32_t getCurrentFrame() const { return m_currentFrame; }

protected:
    OGCamera m_camera;
    std::unordered_map<std::string, std::unique_ptr<IRenderPipeline>> m_pipelines;
    android_app *m_app;
    Input m_input;
    CSMData m_sharedCSMData;
    uint32_t m_currentFrame = 0;
};

#define ENGINE OGSingleton<Engine>::getInstance()


#endif //OXYOUS_2026_ENGINE_HPP
