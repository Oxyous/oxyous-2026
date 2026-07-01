//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#ifndef OXYOUS_2026_OGENTITY_HPP
#define OXYOUS_2026_OGENTITY_HPP

#include "OGObject.hpp"
#include "../../includes.hpp"

#define MAX_COMPONENTS 32

class OGActor;
class OGEntity;

/* Oxyous Game Component */
class OGComponent : public OGObject {
public:
    OGComponent() : OGObject("OGComponent") {
        m_owner = nullptr;
    }
public:
    friend class OGEntity;

    /* Initialize the component */
    virtual void initialize() = 0;

    /* Update the component */
    virtual void update(double deltaTime) = 0;

    /* Destroy the component */
    virtual void destroy() = 0;

    /* Render the component */
    virtual void render(VkCommandBuffer &commandBuffer, uint64_t currentFrame) = 0;

    /* Set the owner of the component */
    [[nodiscard]] virtual OGEntity* getOwner() const {
        return m_owner;
    }
protected:
    OGEntity* m_owner;
};

/* Oxyous Game Entity */
class OGEntity : public OGObject {
public:
    OGEntity() : OGObject("OGEntity") {
        m_parent = nullptr;
        m_rotation = glm::quat(1.0f,0.0f,0.0f,0.0f);
        m_scale = glm::mat4(1.0f);
        m_translation = glm::mat4(1.0f);
        m_worldTransform = glm::mat4(1.0f);
    }
public:

    /* Get Parent of Entity */
    [[nodiscard]] OGEntity* getParent() const {
        return m_parent;
    }

    /* Set Parent of Entity */
    void setParent(OGEntity* parent) {
        m_parent = parent;
    }

    void setName(const std::string& name) {
        m_name = name;
    }

    /* Add Component to Entity */
    template<typename T, typename... TArgs>
    T* addComponent(TArgs&&... args) {
        static_assert(std::is_base_of<OGComponent, T>::value, "T must derive from OGComponent");

        if (hasComponent<T>()) {
            return nullptr;
        }

        T* component = new T(std::forward<TArgs>(args)...);
        component->m_owner = this;
        component->initialize();
        m_components[component->GetType()] = std::unique_ptr<OGComponent>(component);
        return component;
    }

    /* Check if Entity has Component */
    template<typename T>
    bool hasComponent(){
        return m_components.find(T::GetType()) != m_components.end();
    }

    /* Get Component from Entity */
    template<typename T>
    T* getComponent() {
        static_assert(std::is_base_of<OGComponent, T>::value, "T must derive from OGComponent");

        auto it = m_components.find(T::GetType());
        if (it != m_components.end()) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    /* Get All Components attached to Entity */
    template<typename T>
    std::vector<T*> getComponents() {
        static_assert(std::is_base_of<OGComponent, T>::value, "T must derive from OGComponent");

        std::vector<T*> components;
        for (auto& [type, component] : m_components) {
            if (type == T::GetType()) {
                components.push_back(static_cast<T*>(component.get()));
            }
        }
        return components;
    }

    /* Remove Component from Entity */
    template<typename T>
    void freeComponent(){
        static_assert(std::is_base_of<OGComponent, T>::value, "T must derive from OGComponent");
        m_components.erase(T::GetType());
    }

    /* Add Child Entity */
    template<typename T, typename... TArgs>
    T* addChild(TArgs&&... args) {
        static_assert(std::is_base_of<OGEntity, T>::value, "T must derive from OGEntity");

        T* child = new T(static_cast<OGEntity>(std::forward<TArgs>(args))...);

        child->setParent(this);
        m_children.push_back(child);
        return child;
    }

    /* Get children */
    virtual std::vector<OGEntity*> getChildren() {
        return m_children;
    }

    /* Update */
    virtual void update(double deltaTime) {
        for (auto& [type, component] : m_components) {
            component->update(deltaTime);
        }

        for (auto& child : m_children) {
            child->update(deltaTime);
        }
    }

    /* Get World Transform */
    virtual glm::mat4 getWorldTransform() {
        if (m_parent) {
            return m_parent->getWorldTransform() * m_translation * glm::mat4_cast(m_rotation) * m_scale;
        }
        return m_translation * glm::mat4_cast(m_rotation) * m_scale;
    }

    /* Set Local Translation */
    virtual void setTranslation(const glm::vec3& translation) {
        m_translation = glm::translate(glm::mat4(1.0f), translation);
    }

    /* Set Local Rotation */
    virtual void setRotation(const glm::vec3& rotation) {
        m_rotation = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        m_rotation = glm::rotate(m_rotation, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        m_rotation = glm::rotate(m_rotation, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));


    }

    /* Set Local Scale */
    virtual void setScale(const glm::vec3& scale) {
        m_scale = glm::scale(glm::mat4(1.0f), scale);
    }

    /* Get Translation  */
    glm::vec3 getTranslation() const {
        return glm::vec3(m_translation[3]);
    }
protected:
    std::unordered_map<ComponentID, std::unique_ptr<OGComponent>> m_components { MAX_COMPONENTS };
    OGEntity* m_parent;
    std::string m_name;

    glm::mat4 m_translation{};
    glm::quat m_rotation{};
    glm::mat4 m_scale{};

    glm::mat4 m_worldTransform{};

    std::vector<OGEntity*> m_children;

    friend class OGActor;
};


#endif //OXYOUS_2026_OGENTITY_HPP
