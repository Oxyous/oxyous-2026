//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_GAMEVIEW_HPP
#define OXYOUS_2026_GAMEVIEW_HPP

#include "../includes.hpp"
#include "entity/OGEntity.hpp"
#include "collision/Collision.hpp"
#include "input/ThumbStick.hpp"
#include "engine/actors/OGPlayerActor.hpp"
#include "../engine/collision/BHV.hpp"

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
    std::unordered_map<std::string, std::unique_ptr<OGEntity>> &getEntities() {
        return m_entities;
    }

    /* Temporary Collision */
    template<typename T>
    T *addCollider(T *collider) {
        m_colliders.push_back((T *) std::move(collider));
    }

    /** */
    std::vector<std::shared_ptr<IVolume>> &getColliders() {
        return m_colliders;
    }

    /** Debug Ray cast function */
    std::function<void(const Ray &, OGContact &)> raycastCallback;

    /** Add new actor */
    template<typename T, typename... TArgs>
    T *addActor(std::string name, TArgs &&... args) {
        T *actor(new T(std::forward<TArgs>(args)...));

        std::unique_ptr<OGEntity> uPtr{actor};

        m_entities[name] = std::move(uPtr);

        m_entities[name]->initialize();

        return (T *) m_entities[name].get();
    }

    template<typename T>
    T *getActor(const std::string &name) {
        return m_entities.find(name) != m_entities.end() ? (T *) m_entities[name].get() : nullptr;
    }

    /** Get All actors with component */
    template<typename T>
    std::vector<OGEntity *> getActorsWithComponent() {
        std::vector<OGEntity *> results;
        for (auto const &[name, entity]: m_entities) {
            if (entity->hasComponent<T>()) {
                results.push_back(entity.get());
            }
        }
        return results;
    }

    /** Load Scene File */
    bool loadSceneFile(const std::string &sceneFile);

    /** Get World Collision Polygons */
    std::vector<OGPolygon> &getWorldPolygons() {
        return m_worldPolygons;
    }

    /* */
    std::vector<OGPolygon> &getWorldBlockingPolygons() {
        return m_BlockingPolygons;
    }

    /*  */
    void computeBlockingPolygons() {
        m_BlockingPolygons.clear();
        m_BlockingPolygons.reserve(m_worldPolygons.size());
        for (const auto &poly: m_worldPolygons) {
            if (std::abs(poly.normal.y) < 0.5f) {
                m_BlockingPolygons.push_back(poly);
            }
        }
    }

    /** Get Active Player */
    std::shared_ptr<OGActor> getActivePlayer() const {
        return m_activePlayer;
    }

    void setActivePlayer(OGActor *player) {
        const auto playerActor = dynamic_cast<OGPlayerActor*>(player);

        if (!playerActor) {
            aout << "Error: Active player must be of type OGPlayerActor!" << std::endl;
            return;
        }

        // Set the active player using a no-op deleter because m_entities owns the underlying unique_ptr
        m_activePlayer = std::shared_ptr<OGActor>(playerActor, [](OGActor *) {
            /* No-op deleter as m_entities owns it */
        });
    }

    /** Get View Matrix from Active Player */
    glm::mat4 getViewMatrix() const {
        if (m_activePlayer) {
            return dynamic_cast<OGPlayerActor *>(m_activePlayer.get())->getViewMatrix();
        }
        return glm::mat4(1.0f);
    }

    /** Get Projection Matrix from Active Player */
    glm::mat4 getProjectionMatrix() const {
        if (m_activePlayer) {
            return dynamic_cast<OGPlayerActor *>(m_activePlayer.get())->getProjectionMatrix();
        }
        return glm::mat4(1.0f);
    }

    /** Build BHV from polygon list */
    void computeBHV(const std::vector<OGPolygon>& polygons);

    /** Get Possible Polygon intersection */
    void getCapsuleIntersectionByBHV(const CapsuleVolume& capsule, std::vector<OGPolygon>& polygons);

    /** Get Possible Polygons intersection with sphere */
    void getSphereIntersectionByBHV(const SphereVolume& sphere, std::vector<OGPolygon>& polygons);
private:
    /* Temporary Collision */
    std::vector<std::shared_ptr<IVolume>> m_colliders;

    std::unordered_map<std::string, std::unique_ptr<OGEntity>> m_entities;

    std::vector<OGPolygon> m_worldPolygons;
    std::vector<OGPolygon> m_BlockingPolygons;
    std::shared_ptr<OGActor> m_activePlayer;
    std::unique_ptr<BHV> m_bhv;
};

#define GAME_VIEW OGSingleton<GameView>::getInstance()

#endif //OXYOUS_2026_GAMEVIEW_HPP
