//
// Created by Mr Steven J Baldwin on 20/06/2026.
//

#ifndef OXYOUS_2026_GAMEVIEW_HPP
#define OXYOUS_2026_GAMEVIEW_HPP


class IGameView {
public:
    IGameView() = default;
    virtual ~IGameView() = default;

    /* Render Scene Graph */
    virtual void render() = 0;

    /* Update Scene Graph */
    virtual void update(float deltaTime) = 0;

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
    void update(float deltaTime) override;

    /* Initialize Scene Graph */
    bool initialize() override;

    /* Destroy Scene Graph */
    void destroy() override;
private:

};

#define GAME_VIEW OGSingleton<GameView>::getInstance()

#endif //OXYOUS_2026_GAMEVIEW_HPP
