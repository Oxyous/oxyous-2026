//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_GAMEVIEW_HPP
#define OXYOUS_2026_GAMEVIEW_HPP

#include "../includes.hpp"
#include "entity/OGEntity.hpp"
#include "collision/Collision.hpp"
#include "input/ThumbStick.hpp"

class IGameView {
public:
    IGameView() = default;
    virtual ~IGameView() = default;

    /* Render Scene Graph */
    virtual void render() = 0;

    /* Update Scene Graph */
    virtual void update(double deltaTime) = 0;

    /* Initialize Scene Graph */
    virtual bool initialize() = 0;

    /* Destroy Scene Graph */
    virtual void destroy() = 0;
};

class GameView : public IGameView {
public:
    GameView() = default;
    virtual ~GameView() = default;

    /* Render Scene Graph */
    void render() override;

    /* Update Scene Graph */
    void update(double deltaTime) override;

    /* Initialize Scene Graph */
    bool initialize() override;

    /* Destroy Scene Graph */
    void destroy() override;

    /* Get Entities */
    std::unordered_map<std::string, std::unique_ptr<OGEntity>>& getEntities() {
        return m_entities;
    }

    /* Temporary Collision */
    template<typename T>
    T* addCollider(T* collider) {
        m_colliders.push_back((T*)std::move(collider));
    }

    /* */
    std::vector<std::shared_ptr<IVolume>>& getColliders() {
        return m_colliders;
    }

    std::function<void(const Ray&, RaycastHit&)> raycastCallback;

    template<typename T, typename... TArgs>
    T* addActor(std::string name, TArgs&&... args) {
        T* actor(new T(std::forward<TArgs>(args)...));

        std::unique_ptr<OGEntity> uPtr {actor};

        m_entities[name] = std::move(uPtr);

        return (T*)m_entities[name].get();
    }

    template<typename T>
    T* getActor(const std::string& name)
    {
        return m_entities.find(name) != m_entities.end() ? (T*)m_entities[name].get() : nullptr;
    }


private:
    /* Temporary Collision */
    std::vector<std::shared_ptr<IVolume>> m_colliders;

    std::unordered_map<std::string, std::unique_ptr<OGEntity>> m_entities;
};

#define GAME_VIEW OGSingleton<GameView>::getInstance()

#endif //OXYOUS_2026_GAMEVIEW_HPP
