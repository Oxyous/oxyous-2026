//
// Created by Mr Steven J Baldwin on 08/07/2026.
//

#ifndef OXYOUS_2026_OGSPRITE_HPP
#define OXYOUS_2026_OGSPRITE_HPP


#include "engine/elements/OGElement.hpp"
#include <string>

class OGSprite : public OGElement {
public:
    OGSprite(const std::string& texturePath) : m_texturePath(texturePath) {}

    void draw(VkCommandBuffer& commandBuffer, uint32_t frame) override;




private:
    std::string m_texturePath;
};


#endif //OXYOUS_2026_OGSPRITE_HPP
