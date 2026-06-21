//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_ENGINE_HPP
#define OXYOUS_2026_ENGINE_HPP

#include "../includes.hpp"
#include "../system/OGSingleton.hpp"
#include "../render/vulkan/pipelines/IRenderPipeline.hpp"

class Engine {
public:
    Engine() = default;
    virtual ~Engine() = default;
public:
    /* Initialize Game Engine */
    virtual bool initialize();

    /* Destroy Game Engine */
    virtual void destroy();

    /*  Update Game Engine */
    virtual void update(float deltaTime);

    /*  Render Game Engine */
    virtual void render();

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

protected:
    std::unordered_map<std::string, std::unique_ptr<IRenderPipeline>> m_pipelines;
};

#define ENGINE OGSingleton<Engine>::getInstance()


#endif //OXYOUS_2026_ENGINE_HPP
