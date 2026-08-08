//
// Created by Mr Steven J Baldwin on 08/08/2026.
//

#ifndef OXYOUS_2026_UIWIDGET_HPP
#define OXYOUS_2026_UIWIDGET_HPP

#include "../../includes.hpp"
#include "../ui/OGUi.hpp"
#include "../../DataStructures.hpp"

template<typename T, typename V>
inline static void setProperty(T &obj, V T::*member, const V &value) {
    obj.*member = value;
}

struct UIProperties
{
    std::function<void()> onTapCallback;
    std::string atlasSprite;
    glm::vec2 position;
    glm::vec2 screenSize;
    glm::vec4 colorAlpha;
};

class UIInterface {
public:
    virtual ~UIInterface() = default;

    /** Create Element (Sprite, Button, etc.) */
    virtual void create(const UIProperties &property) = 0;

    virtual bool handleInput(const glm::vec2 &touchPosition, bool pressed) = 0;

    virtual void setParent(std::shared_ptr<UIInterface> parent) = 0;

    virtual void addChild(std::shared_ptr<UIInterface> child) = 0;

    virtual void setSpriteInstance(const SpriteInstance& instance) {
        m_spriteInstance = instance;
    }

    virtual SpriteInstance& getSpriteInstance() {
        return m_spriteInstance;
    }

protected:
    SpriteInstance m_spriteInstance;
};


/** Base UI Widget Class */
class UIWidget : public UIInterface, public std::enable_shared_from_this<UIWidget> {
public:
    virtual ~UIWidget() = default;

    /** Create Element (Sprite, Button, etc.) */
    void create(const UIProperties &property) override = 0;

    /** Process inputs */
    bool handleInput(const glm::vec2 &touchPosition, bool pressed) override = 0;

    /** Set parent element */
    void setParent(std::shared_ptr<UIInterface> parent) override {
        m_parent = std::dynamic_pointer_cast<UIWidget>(parent);
    }

    /** Add Child element*/
    void addChild(std::shared_ptr<UIInterface> child) override {
        if (auto widget = std::dynamic_pointer_cast<UIWidget>(child)) {
            widget->setParent(shared_from_this());
            m_children.push_back(std::move(widget));
        }
    }

public:
    std::function<void()> m_onTapCallback;

    template<typename T, typename... Args>
    T &addWidget(Args &&... args) {
        auto element = std::make_shared<T>(std::forward<Args>(args)...);

        T &ref = *element;
        element->setParent(shared_from_this());
        m_children.push_back(std::move(element));

        return ref;
    }

protected:
    std::string m_name;
    std::string m_texture;
    std::weak_ptr<UIWidget> m_parent;
    std::vector<std::shared_ptr<UIWidget>> m_children;
};

/** */
class UIButtonWidget : public UIWidget {
public:
    UIButtonWidget() = default;
    ~UIButtonWidget() override = default;

    UIButtonWidget(const UIProperties &property) {
        create(property);
    }

    void create(const UIProperties &property) override {
        // Implementation for creating a button
        m_spriteInstance.position = property.position;
        m_spriteInstance.size = property.screenSize;
        m_spriteInstance.colorAlpha = property.colorAlpha;
        m_texture = property.atlasSprite;
        m_onTapCallback = property.onTapCallback;
        UI->addSprite(m_texture, m_spriteInstance.position, m_spriteInstance.size, m_spriteInstance.colorAlpha);
    }

    bool handleInput(const glm::vec2 &touchPosition, bool pressed) override {
        // Implementation for handling input for the button
        if (touchPosition.x >= m_spriteInstance.position.x && touchPosition.x <= m_spriteInstance.position.x + m_spriteInstance.size.x &&
            touchPosition.y >= m_spriteInstance.position.y && touchPosition.y <= m_spriteInstance.position.y + m_spriteInstance.size.y) {
            if (!pressed && m_onTapCallback) {
                m_onTapCallback();
            }
            return true;
        }
        return false;
    }

    void setParent(std::shared_ptr<UIInterface> parent) override {
        UIWidget::setParent(parent);
    }

    void addChild(std::shared_ptr<UIInterface> child) override {
        UIWidget::addChild(child);
    }

    UIButtonWidget &setOnTapCallback(const std::function<void()> &callback) {
        m_onTapCallback = callback;
        return *this;
    }
};

/** UI List Collection of Widgets */
class UIList : public UIWidget {
public:
    void create(const UIProperties &property) override {
        // Implementation for creating a UI list
    }

    bool handleInput(const glm::vec2 &touchPosition, bool pressed) override {
        // Implementation for handling input for the UI list
        return false;
    }

    std::vector<std::shared_ptr<UIWidget>> &getElements() {
        return m_children;
    }
};

/** UI Base Menu */
class UIMenu : public UIWidget {
public:
    void create(const UIProperties &property) override {
        // Implementation for creating a menu
    }

    /** */
    bool handleInput(const glm::vec2 &touchPosition, bool pressed) override {
        /** */
        for (const auto &element: m_children) {
            if (element->handleInput(touchPosition, pressed)) {
                return true;
            }
        }

        return false;
    }

    void setParent(std::shared_ptr<UIInterface> parent) override {
        UIWidget::setParent(parent);
    }

    void addChild(std::shared_ptr<UIInterface> child) override {
        m_children.push_back(std::dynamic_pointer_cast<UIWidget>(child));
        repositionItems();
    }

    void addOption(std::shared_ptr<UIWidget> item) {
        m_children.push_back(item);
        repositionItems();
    }

private:

    void repositionItems() {
        float y = 0.0f;
        for (const auto &item: m_children) {
            item->getSpriteInstance().position.y = y;
            y += item->getSpriteInstance().size.y;
        }
    }
};

/** */

#endif //OXYOUS_2026_UIWIDGET_HPP
