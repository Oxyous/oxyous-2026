//
// Created by Mr Steven J Baldwin on 08/08/2026.
//

#ifndef OXYOUS_2026_UILAYER_HPP
#define OXYOUS_2026_UILAYER_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include "UIWidget.hpp"

class UILayer {
public:
    UILayer(std::string name) : m_name(std::move(name)) {}

    /** Add sprite instance id to layer */
    void addInstance(const SpriteInstance& instance) {
        if (std::find(m_instance.begin(), m_instance.end(), instance) == m_instance.end()) {
            m_instance.push_back(instance);
        }
    }

    /** Remove sprite instance from layer */
    void removeInstance(const SpriteInstance& instance) {
        m_instance.erase(std::remove(m_instance.begin(), m_instance.end(), instance), m_instance.end());
    }

    /** Clear all instances from layer */
    void clear() {
        m_instance.clear();
    }

    /** Get All Sprite Instances in this layer */
    const std::vector<SpriteInstance>& getInstances() const {
        return m_instance;
    }

    /** Get Layer Name */
    const std::string& getName() const {
        return m_name;
    }

    /** Add Child Element */
    void addChild(std::shared_ptr<UIInterface> child) {
        if (std::find(m_instance.begin(), m_instance.end(), child->getSpriteInstance()) == m_instance.end()) {
            m_instance.push_back(child->getSpriteInstance());
        }
        m_children.push_back(std::move(child));
    }

    /** Remove Child Element */
    void removeChild(std::shared_ptr<UIInterface> child) {
        m_children.erase(std::remove(m_children.begin(), m_children.end(), child), m_children.end());
    }

    /** Get Child Elements */
    const std::vector<std::shared_ptr<UIInterface>>& getChildren() const {
        return m_children;
    }

    /** Clear all children */
    void clearChildren() {
        m_children.clear();
    }

    bool handleInput(const glm::vec2 &touchPosition, bool pressed) {
        for (const auto& child : m_children) {
            if (child->handleInput(touchPosition, pressed)) {
                return true;
            }
        }
        return false;
    }

private:
    std::string m_name;
    std::vector<SpriteInstance> m_instance;
    std::vector<std::shared_ptr<UIInterface>> m_children;
};

#endif //OXYOUS_2026_UILAYER_HPP
