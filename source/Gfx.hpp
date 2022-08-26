#pragma once

#include "Utils.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <whb/gfx.h>
#include <gx2/context.h>

class Gfx {
public:
    enum Target {
        TARGET_TV,
        TARGET_DRC0,
        TARGET_DRC1,

        NUM_TARGETS,
    };

    struct Texture {
        GX2Texture texture;
        GX2Sampler sampler;
        // xy: offset, zw: scale
        float texCoordParams[4];

        uint32_t GetPitch();

        glm::uvec2 GetSize();

        void SetClamp(bool clamp);

        void SetLinearFilter(bool linear);

        void SetUVOffset(glm::vec2 offset);

        void SetUVScale(glm::vec2 scale);

        void Update(void* rgba);

        void* Lock();

        void Unlock();

        void Delete();

    private:
        friend Gfx;
        Texture() = default;
        ~Texture() = default;
    };

    Gfx();
    virtual ~Gfx();

    bool Initialize();

    void Finalize();

    void SetModel(glm::mat4& model);

    void SetView(glm::mat4& view);

    void BeginDraw(Target target, glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    void EndDraw();

    void Draw(Texture* tex, const void* vertices, uint32_t numVertices, glm::vec4 color = glm::vec4(1.0f), bool quads = false);

    void SwapBuffers(void);

    static Texture* NewTexture(glm::uvec2 size, void* rgba = nullptr, bool clamp = false, bool linearFilter = true);

    // virtual screen space used in projection
    static inline glm::vec2 screenSpace = glm::vec2(1280.0f, 720.0f);

private:
    static uint32_t ProcUiAcquired(void* arg);
    static uint32_t ProcUiReleased(void* arg);
    int OnForegroundAcquired();
    int OnForegroundReleased();

    bool inForeground;
    void* commandBufferPool;

    GX2TVRenderMode tvRenderMode;
    glm::uvec2 tvSize;
    uint32_t tvScanBufferSize;
    void* tvScanBuffer;

    GX2DrcRenderMode drcRenderMode;
    glm::uvec2 drcSize;
    uint32_t drcScanBufferSize;
    void* drcScanBuffer;

    GX2ColorBuffer colorBuffers[NUM_TARGETS];

    GX2ContextState* contextState;

    Target currentTarget;

    bool displaysEnabled;

    bool matrixUpdated;
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    enum Shader {
        SHADER_INVALID = -1,

        SHADER_COLOR,
        SHADER_TEXTURE,

        NUM_SHADERS,
    };

    WHBGfxShaderGroup shaderGroups[NUM_SHADERS];
    Shader currentShader;
};
