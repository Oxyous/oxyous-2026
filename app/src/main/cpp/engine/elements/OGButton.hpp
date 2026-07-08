//
// Created by Mr Steven J Baldwin on 08/07/2026.
//

#ifndef OXYOUS_2026_OGBUTTON_HPP
#define OXYOUS_2026_OGBUTTON_HPP

#include "OGElement.hpp"

class OGButton {
public:

    OGButton(std::string name, std::string spriteName, glm::vec2 position, glm::vec2 size, std::function<void()> onTap) {
        create(position, size, name, spriteName);
        setOnTap(onTap);
    }

    void create(const glm::vec2 &origin, const glm::vec2 &size, const std::string &text, const std::string &spriteName);

    bool handleInput(const glm::vec2& touchPosition, bool pressed);

    void setOnTap(std::function<void()> onTap);

protected:
    std::string m_text;
    glm::vec2 m_position;
    glm::vec2 m_size;
    uint32_t m_spriteInstanceId;
    std::function<void()> m_onTap;
};

#endif //OXYOUS_2026_OGBUTTON_HPP
