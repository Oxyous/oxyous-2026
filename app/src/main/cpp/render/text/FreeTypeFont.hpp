//
// Created by Mr Steven J Baldwin on 06/07/2026.
//

#ifndef OXYOUS_2026_FREETYPEFONT_HPP
#define OXYOUS_2026_FREETYPEFONT_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"

class FreeTypeFont {
public:
    FreeTypeFont();
    ~FreeTypeFont();

    /* Initialize font */
    bool initializeFont(const std::string& fontFile);

    /* Render String */
    void renderString(VkCommandBuffer& cmd, const std::string& text, float x, float y, float scale);

private:
    FT_Library m_ftLibrary;
    FT_Face m_ftFace;
    std::map<char, OGChar> m_characters;
    GPUBuffer m_vertexBuffer;
    GPUBuffer m_indexBuffer;

    static constexpr int MAX_CHARACTERS = 1000;
};


#endif //OXYOUS_2026_FREETYPEFONT_HPP
