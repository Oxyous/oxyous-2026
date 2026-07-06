//
// Created by Mr Steven J Baldwin on 06/07/2026.
//

#include "FreeTypeFont.hpp"
#include "../vulkan/RenderFramework.hpp"
#include "engine/elements/ScreenSpaceRenderer.hpp"
#include "engine/GPUResources.hpp"


FreeTypeFont::FreeTypeFont()
        : m_ftLibrary(nullptr), m_ftFace(nullptr) {
}

/* Initialize font */
bool FreeTypeFont::initializeFont(const std::string &fontFile) {

    std::vector<uint8_t> fontData;

    if (!RESOURCE_MANAGER->loadBinary<uint8_t>(fontFile, fontData)) {
        throw std::runtime_error("Error: Could not load font file: " + fontFile);
        return false;
    }

    if (FT_Init_FreeType(&m_ftLibrary)) {
        throw std::runtime_error("Error: Could not initialize FreeType library");
        return false;
    }

    if (FT_New_Memory_Face(m_ftLibrary, fontData.data(), static_cast<FT_Long>(fontData.size()), 0,
                           &m_ftFace)) {
        throw std::runtime_error("Error: Could not load font face");
        return false;
    }

    if (FT_Set_Pixel_Sizes(m_ftFace, 0, 48)) {
        throw std::runtime_error("Error: Failed to set pixel size");
        return false;
    }

    if (FT_Load_Char(m_ftFace, 'X', FT_LOAD_RENDER)) {
        throw std::runtime_error("Error: Failed to load character 'X'");
        return false;
    }

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(m_ftFace, c, FT_LOAD_RENDER)) {
            throw std::runtime_error("Error: Failed to load character");
            return false;
        }

        auto bufferSize = m_ftFace->glyph->bitmap.width * m_ftFace->glyph->bitmap.rows;
        const int glyphWidth = static_cast<int>(m_ftFace->glyph->bitmap.width);
        const int glyphHeight = static_cast<int>(m_ftFace->glyph->bitmap.rows);
        const uint32_t glyphPixelCount = static_cast<uint32_t>(glyphWidth * glyphHeight);

        std::vector<uint8_t> rgbaPixels;
        int uploadWidth = glyphWidth;
        int uploadHeight = glyphHeight;

        if (glyphPixelCount > 0) {
            rgbaPixels.resize(glyphPixelCount * 4);
            const FT_Bitmap& bmp = m_ftFace->glyph->bitmap;
            const int pitch = std::abs(bmp.pitch);

            for (int row = 0; row < glyphHeight; ++row) {
                for (int col = 0; col < glyphWidth; ++col) {
                    const uint8_t alpha = bmp.buffer[row * pitch + col];
                    const uint32_t i = static_cast<uint32_t>(row * glyphWidth + col) * 4;
                    rgbaPixels[i + 0] = 255;
                    rgbaPixels[i + 1] = 255;
                    rgbaPixels[i + 2] = 255;
                    rgbaPixels[i + 3] = alpha;
                }
            }
        } else {
            // Keep texture creation valid for whitespace/empty glyphs.
            uploadWidth = 1;
            uploadHeight = 1;
            rgbaPixels = {255, 255, 255, 0};
        }

        std::shared_ptr<GPUTexture> texture = std::make_shared<GPUTexture>();
        if (!RenderFramework::createGpuTexture(rgbaPixels.data(), rgbaPixels.size(),
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               uploadWidth,
                                               uploadHeight, texture)) {
            return false;
        }

        GPUElementHandle handle;
        handle.textureId = SCREEN_RENDER->registerTexture(*texture);
        handle.transform = glm::mat4(1.0f);

        OGChar character = {
                handle.textureId,
                SCREEN_RENDER->registerObject(handle),
                glm::ivec2(m_ftFace->glyph->bitmap.width, m_ftFace->glyph->bitmap.rows),
                glm::ivec2(m_ftFace->glyph->bitmap_left, m_ftFace->glyph->bitmap_top),
                static_cast<uint32_t>(m_ftFace->glyph->advance.x)
        };
        m_characters.insert(std::pair<char, OGChar>(c, character));
    }

    FT_Done_Face(m_ftFace);
    FT_Done_FreeType(m_ftLibrary);

    /* Prepare Vertex Buffers for strings */
    RenderFramework::createBuffer(sizeof(OGVertex2D) * 4 * MAX_CHARACTERS,
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  nullptr, &m_vertexBuffer);


    std::vector<OGVertex2D> vertices;
    vertices.resize(4);

    // Top Left
    vertices[0].position = glm::vec2(-1.0f, -1.0f);
    vertices[0].uv = glm::vec2(0.0f, 0.0f);

    // Top Right
    vertices[1].position = glm::vec2(1.0f, -1.0f);
    vertices[1].uv = glm::vec2(1.0f, 0.0f);

    // Bottom Right
    vertices[2].position = glm::vec2(1.0f, 1.0f);
    vertices[2].uv = glm::vec2(1.0f, 1.0f);

    // Bottom Left
    vertices[3].position = glm::vec2(-1.0f, 1.0f);
    vertices[3].uv = glm::vec2(0.0f, 1.0f);


    void *data;
    vkMapMemory(RENDER_DEVICE->getDevice(), m_vertexBuffer.memory, 0,
                vertices.size() * sizeof(OGVertex2D), 0, &data);
    memcpy(data, vertices.data(), vertices.size() * sizeof(OGVertex2D));
    vkUnmapMemory(RENDER_DEVICE->getDevice(), m_vertexBuffer.memory);


    return true;
}

FreeTypeFont::~FreeTypeFont() {
    if (m_ftFace) {
        FT_Done_Face(m_ftFace);
    }
    if (m_ftLibrary) {
        FT_Done_FreeType(m_ftLibrary);
    }
}

void FreeTypeFont::renderString(VkCommandBuffer &cmd, const std::string &text, float x, float y,
                                float scale) {

    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer.buffer, &m_vertexBuffer.offset);


    for (const char &c: text) {
        auto it = m_characters.find(c);
        if (it == m_characters.end()) continue;
        const OGChar& ch = it->second;

        float xpos = x + ch.bearing.x * scale;
        float ypos = y + (ch.size.y - ch.bearing.y) * scale;
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        GPUElementHandle handle{};
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(xpos + w / 2.0f, ypos + h / 2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(w / 2.0f, h / 2.0f, 1.0f));
        handle.transform = model;
        handle.textureId = ch.textureId;
        SCREEN_RENDER->updateElement(ch.objectId, handle);

        PCScreenElements pc = {};
        pc.objectIndex = ch.objectId;
        pc.textureIndex = ch.textureId;
        vkCmdPushConstants(cmd, SCREEN_RENDER->getPipelineLayout(),
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(PCScreenElements), &pc);

        vkCmdDraw(cmd, static_cast<uint32_t>(4), 1, 0, 0);

        x += (ch.advance >> 6) * scale;

    }

}

