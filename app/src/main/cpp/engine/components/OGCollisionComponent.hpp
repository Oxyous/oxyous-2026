//
// Created by Mr Steven J Baldwin on 04/07/2026.
//

#ifndef OXYOUS_2026_OGCOLLISIONCOMPONENT_HPP
#define OXYOUS_2026_OGCOLLISIONCOMPONENT_HPP


#include "../entity/OGEntity.hpp"
#include "../collision/Collision.hpp"

class OGCollisionComponent : public OGComponent {
public:


    GET_UNIQUE_TYPE(OGCollisionComponent)

    void initialize() override;

    void update(double deltaTime) override;

    void destroy() override;

    void render(VkCommandBuffer &commandBuffer, uint64_t currentFrame) override;

    void setVolume(std::unique_ptr<IVolume> volume);

    template<typename T>
    T* getCollisionVolume(){
        return dynamic_cast<T*>(m_collider.get());
    }

private:
    std::unique_ptr<IVolume> m_collider;
};


#endif //OXYOUS_2026_OGCOLLISIONCOMPONENT_HPP
