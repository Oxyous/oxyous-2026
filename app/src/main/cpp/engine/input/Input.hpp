//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_INPUT_HPP
#define OXYOUS_2026_INPUT_HPP

#include "../../includes.hpp"
#include "ThumbStick.hpp"

class IInput {
public:
    IInput() = default;

    virtual ~IInput() = default;

    virtual void update(double delta) = 0;

public:
    virtual void handleInput() = 0;
};

typedef struct TouchEvent {
    enum {
        started,
        moved,
        stopped,
        invalid
    } eType = invalid;

    int32_t counter;

    struct TouchPoint {
        uintptr_t identifier;
        glm::vec2 position;
        glm::vec2 rawPosition;
        bool isChanged = false;
        std::chrono::steady_clock::time_point time;
    } touchPoints[4];
} TouchEvent;

/* System input */
class Input : public IInput {
public:
    Input();

    virtual ~Input() = default;

public:
    void handleInput() override;

    bool initialize();

    void update(double delta) override;

    /* Add Thumbstick */
    void addThumbStick(const std::shared_ptr<ThumbStick>& thumbStick) {
        m_thumbSticks.push_back(thumbStick);
    }

    /* Get thumbstick */
    ThumbStick* getThumbStick(ThumbStickType type) {
        for (const auto& stick : m_thumbSticks) {
            if (stick->getType() == type) {
                return stick.get();
            }
        }
        return nullptr;
    }

    void processTap(TouchEvent& event, uint32_t i);

    /* Process Input events */
    void processEvents(TouchEvent& event);
public:
    glm::vec2 getPosition();
protected:
    std::map<int, ThumbStick*> touchStack(TouchEvent& event);

    glm::vec2 m_position;
    glm::vec2 m_oldPosition;

    glm::vec2 m_tapStartPosition;
    double m_tapStartTime = 0.0;

    std::vector<std::shared_ptr<ThumbStick>> m_thumbSticks;
    std::map<uintptr_t, std::shared_ptr<ThumbStick>> m_thumbStickMap;
    std::map<uintptr_t, std::chrono::steady_clock::time_point> m_touchStartTimes;
    std::queue<ThumbStick*> m_processQueue;
};

#endif //OXYOUS_2026_INPUT_HPP
