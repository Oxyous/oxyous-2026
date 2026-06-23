//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_OGACTOR_HPP
#define OXYOUS_2026_OGACTOR_HPP


#include "../entity/OGEntity.hpp"

class OGActor : public OGEntity {
public:
    OGActor(const std::string& name) : OGEntity(name) {}
public:
    void setPath(const std::vector<glm::vec3>& path) {
        m_path = path;
        m_pathIndex = 0;
    }

    void update(double deltaTime) override {
        OGEntity::update(deltaTime);

        if (m_path.size() > 0) {
            glm::vec3 targetPosition = m_path[m_pathIndex];
            glm::vec3 direction = targetPosition - getTranslation();
            float distance = glm::length(direction);

            if (distance < 0.1f) {
                // Move to the next point in the path
                m_pathIndex = (m_pathIndex + 1) % m_path.size();

                if (m_pathIndex == 0) {
                    // Reached the end of the path
                    setPath(std::vector<glm::vec3>());
                }

            } else {
                // Normalize the direction and move towards the target position
                direction = glm::normalize(direction);
                setTranslation(getTranslation() + direction * speed * static_cast<float>(deltaTime));
            }
        }
    }

protected:
    std::vector<glm::vec3> m_path;
private:
    float speed = 1.0f;
    std::size_t m_pathIndex = 0;
};


#endif //OXYOUS_2026_OGACTOR_HPP
