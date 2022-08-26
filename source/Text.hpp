#pragma once

#include "Gfx.hpp"
#include "Sprite.hpp"

#include <string>

class Text : public Sprite {
public:
    static void InitializeFont();
    static void DeinitializeFont();

public:
    Text(std::string text, uint32_t textSize = 24, glm::vec2 pos = glm::vec2(), glm::vec2 scale = glm::vec2(1.0f), float angle = 0.0f, glm::vec4 color = glm::vec4(1.0f));
    virtual ~Text();

    void SetText(std::string text);

    void SetTextSize(uint32_t textSize);

private:
    void RedrawTexture();

    glm::uvec2 bounds;
    Gfx::Texture* texture;

    std::wstring text;
    uint32_t textSize;
};
