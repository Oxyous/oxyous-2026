//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_OGACTOR_HPP
#define OXYOUS_2026_OGACTOR_HPP


#include "../entity/OGEntity.hpp"

class OGActor : public OGEntity {
public:
    OGActor() : OGEntity() {}

public:
    void setPath(const std::vector<glm::vec3> &path) {
        m_path = path;
        m_pathIndex = 0;
    }

    void update(double deltaTime) override {
        OGEntity::update(deltaTime);

        if(m_path.size() > 0) {
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
                setTranslation(
                        getTranslation() + direction * speed * static_cast<float>(deltaTime));

                auto newDirection = glm::quat_cast(
                        lookRotation(direction, glm::vec3(0.0f, 1.0f, 0.0f)));

                m_rotation = glm::lerp(glm::normalize(m_rotation), newDirection,
                                       (float) deltaTime * 5.0f); // Smoothly interpolate rotation

            }
        }
    }

    glm::mat4 lookRotation(glm::vec3 direction, glm::vec3 up) {
        glm::vec3 zAxis = glm::normalize(direction);
        glm::vec3 xAxis = glm::normalize(glm::cross(up, zAxis));
        glm::vec3 yAxis = glm::cross(zAxis, xAxis);

        glm::mat4 rotationMatrix(1.0f);
        rotationMatrix[0] = glm::vec4(xAxis, 0.0f);
        rotationMatrix[1] = glm::vec4(yAxis, 0.0f);
        rotationMatrix[2] = glm::vec4(zAxis, 0.0f);

        return rotationMatrix;
    }

protected:
    std::vector<glm::vec3> m_path;
private:
    float speed = 3.0f;
    std::size_t m_pathIndex = 0;
};


#endif //OXYOUS_2026_OGACTOR_HPP
