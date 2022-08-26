#pragma once

#include "Gfx.hpp"

class Sprite {
public:
    static Sprite* FromPNG(const void* data, uint32_t size);

public:
    Sprite(glm::vec2 position = glm::vec2(), glm::vec2 size = glm::vec2(), float angle = 0.0f, glm::vec4 color = glm::vec4(1.0f));
    Sprite(Gfx::Texture* texture, glm::vec2 position = glm::vec2(), float angle = 0.0f, glm::vec4 color = glm::vec4(1.0f));
    virtual ~Sprite();

    void SetTexture(Gfx::Texture* texture, bool updateSize = false);

    void SetPosition(glm::vec2 pos);

    void SetSize(glm::vec2 size);

    void SetScale(glm::vec2 scale);

    void SetAngle(float angle);

    void SetColor(glm::vec4 color);

    void SetCentered(bool centered);

    void SetVisible(bool visible);

    void SetUVOffset(glm::vec2 off);

    void SetUVScale(glm::vec2 scale);

    void SetLinearFilter(bool linear);

    glm::vec2 const& GetPosition() const;

    glm::vec2 const& GetSize() const;

    glm::vec2 const& GetScaledSize() const;

    float GetAngle() const;

    glm::vec4 const& GetColor() const;

    glm::vec2 GetForwardVector() const;

    virtual void Draw(Gfx* gfx);

protected:
    void UpdateModel();

    bool deleteTexture;
    Gfx::Texture* texture;

    glm::mat4 model;

    glm::vec2 position;
    glm::vec2 scaledSize;
    glm::vec2 size;
    glm::vec2 scale;
    float angle;
    glm::vec4 color;
    bool centered;
    bool visible;
};
