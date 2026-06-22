//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_GAMEVIEW_HPP
#define OXYOUS_2026_GAMEVIEW_HPP

#include "../includes.hpp"
#include "entity/OGEntity.hpp"

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
    std::vector<std::shared_ptr<OGEntity>>& getEntities() {
        return m_entities;
    }

private:
    std::vector<std::shared_ptr<OGEntity>> m_entities;
};

#define GAME_VIEW OGSingleton<GameView>::getInstance()

#endif //OXYOUS_2026_GAMEVIEW_HPP
