#include "Sprite.hpp"

#include <png.h>

// Aligned vertex buffers which can be directly sent to the GPU
static float colorVertices[][2] __attribute__ ((aligned (GX2_VERTEX_BUFFER_ALIGNMENT))) = {
    { 0.0f, 1.0f, },
    { 1.0f, 0.0f, },
    { 0.0f, 0.0f, }, 
    { 0.0f, 1.0f, },
    { 1.0f, 1.0f, },
    { 1.0f, 0.0f, },
};

static const float textureVertices[][4] __attribute__ ((aligned (GX2_VERTEX_BUFFER_ALIGNMENT))) = {
    { 0.0f, 1.0f, 0.0f, 1.0f, },
    { 1.0f, 0.0f, 1.0f, 0.0f, },
    { 0.0f, 0.0f, 0.0f, 0.0f, },
    { 0.0f, 1.0f, 0.0f, 1.0f, },
    { 1.0f, 1.0f, 1.0f, 1.0f, },
    { 1.0f, 0.0f, 1.0f, 0.0f, },
};

static void png_read_data(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
{
    void** data = (void**) png_get_io_ptr(png_ptr);

    memcpy(outBytes, *data, byteCountToRead);
    *((uint8_t**) data) += byteCountToRead;
}

Sprite* Sprite::FromPNG(const void* data, uint32_t size)
{
    Sprite* s = new Sprite();

    // Setup png read and info struct
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        delete s;
        return nullptr;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        delete s;
        return nullptr;
    }

    // Set read function and info
    png_set_read_fn(png_ptr, (void *) &data, png_read_data);
    png_read_info(png_ptr, info_ptr);

    // Read the IHDR
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    int bitDepth = 0;
    int colorType = -1;
    if (png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr) != 1) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        delete s;
        return nullptr;
    }

    // convert RGB data to RGBA
    if (colorType == PNG_COLOR_TYPE_RGB) {
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }

    // Create the texture
    Gfx::Texture* tex = Gfx::NewTexture(glm::uvec2(width, height));
    if (!tex) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        delete s;
        return nullptr;
    }

    s->SetTexture(tex, true);

    // Read the png data into the texture
    uint32_t pitch = tex->GetPitch();
    uint8_t* textureData = (uint8_t*) tex->Lock();
    for (png_uint_32 y = 0; y < height; y++) {
        png_read_row(png_ptr, (png_bytep) textureData + (y * pitch * 4), nullptr);
    }
    tex->Unlock();

    // Cleanup
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    return s;
}

Sprite::Sprite(glm::vec2 position, glm::vec2 size, float angle, glm::vec4 color) :
    deleteTexture(false),
    texture(nullptr),
    position(position),
    size(size),
    scale(1.0f),
    angle(angle),
    color(color),
    centered(false),
    visible(true)
{
    // Calculate scaled size
    this->scaledSize = size * scale;

    // Calculate the model matrix
    UpdateModel();
}

Sprite::Sprite(Gfx::Texture* texture, glm::vec2 position, float angle, glm::vec4 color) :
    deleteTexture(false),
    texture(texture),
    position(position),
    scale(1.0f),
    angle(angle),
    color(color),
    centered(false),
    visible(true)
{
    // Get the size from the texture
    SetSize(texture->GetSize());
}

Sprite::~Sprite()
{
    if (texture && deleteTexture) {
        texture->Delete();
    }
}

void Sprite::SetTexture(Gfx::Texture* texture, bool updateSize)
{
    this->texture = texture;

    // Set the size of the sprite to match the texture size if wanted
    if (updateSize) {
        SetSize(texture->GetSize());
    }
}

void Sprite::SetPosition(glm::vec2 pos)
{
    this->position = pos;
    UpdateModel();
}

void Sprite::SetSize(glm::vec2 size)
{
    this->size = size;
    this->scaledSize = size * scale;
    UpdateModel();
}

void Sprite::SetScale(glm::vec2 scale)
{
    this->scale = scale;
    this->scaledSize = size * scale;
    UpdateModel();
}

void Sprite::SetAngle(float angle)
{
    this->angle = angle;
    UpdateModel();
}

void Sprite::SetColor(glm::vec4 color)
{
    this->color = color;
    UpdateModel();
}

void Sprite::SetCentered(bool centered)
{
    this->centered = centered;
    UpdateModel();
}

void Sprite::SetVisible(bool visible)
{
    this->visible = visible;
}

void Sprite::SetUVOffset(glm::vec2 off)
{
    if (texture) {
        texture->SetUVOffset(off);
    }
}

void Sprite::SetUVScale(glm::vec2 scale)
{
    if (texture) {
        texture->SetUVScale(scale);
    }
}

void Sprite::SetLinearFilter(bool linear)
{
    if (texture) {
        texture->SetLinearFilter(linear);
    }
}

glm::vec2 const& Sprite::GetPosition() const
{
    return position;
}

glm::vec2 const& Sprite::GetSize() const
{
    return size;
}

glm::vec2 const& Sprite::GetScaledSize() const
{
    return scaledSize;
}

float Sprite::GetAngle() const
{
    return angle;
}

glm::vec4 const& Sprite::GetColor() const
{
    return color;
}

glm::vec2 Sprite::GetForwardVector() const
{
    glm::vec2 forwardVector;
    forwardVector.x = sin(glm::radians(angle));
    forwardVector.y = -cos(glm::radians(angle));
    return glm::normalize(forwardVector);
}

void Sprite::Draw(Gfx* gfx)
{
    // No need to draw if the sprite is not visible
    if (!visible) {
        return;
    }

    // Set model matrix
    gfx->SetModel(model);

    // draw the sprite
    if (texture) {
        gfx->Draw(texture, textureVertices, 6, color);
    } else {
        gfx->Draw(nullptr, colorVertices, 6, color);
    }
}

// Update the model matrix
void Sprite::UpdateModel()
{
    // Reset model matrix
    model = glm::mat4(1.0f);

    // Setup positions
    model = glm::translate(model, glm::vec3(position, 0.0f));

    // If we're centered move the coords upwards so the position is the center
    if (centered) {
        model = glm::translate(model, glm::vec3(-0.5f * scaledSize.x, -0.5f * scaledSize.y, 0.0f));
    }

    // Center coords and rotate
    model = glm::translate(model, glm::vec3(0.5f * scaledSize.x, 0.5f * scaledSize.y, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-0.5f * scaledSize.x, -0.5f * scaledSize.y, 0.0f));

    // Scale
    model = glm::scale(model, glm::vec3(scaledSize, 1.0f));
}
