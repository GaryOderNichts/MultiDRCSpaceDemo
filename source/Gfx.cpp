#include "Gfx.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <gx2/clear.h>
#include <gx2/display.h>
#include <gx2/draw.h>
#include <gx2/event.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/state.h>
#include <gx2/swap.h>
#include <gx2/utils.h>

#include <coreinit/memfrmheap.h>
#include <proc_ui/procui.h>

#include <malloc.h>

#include "colorShader_gsh.h"
#include "textureShader_gsh.h"

static void InitColorBuffer(GX2ColorBuffer& cb, glm::uvec2& size, GX2SurfaceFormat format)
{
    memset(&cb, 0, sizeof(GX2ColorBuffer));
    cb.surface.use = GX2_SURFACE_USE_TEXTURE_COLOR_BUFFER_TV;
    cb.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    cb.surface.width = size.x;
    cb.surface.height = size.y;
    cb.surface.depth = 1;
    cb.surface.mipLevels = 1;
    cb.surface.format = format;
    cb.surface.tileMode = GX2_TILE_MODE_DEFAULT;
    cb.viewNumSlices = 1;
    GX2CalcSurfaceSizeAndAlignment(&cb.surface);
    GX2InitColorBufferRegs(&cb);
}

Gfx::Gfx()
{
    inForeground = false;
    displaysEnabled = false;

    currentShader = SHADER_INVALID;

    modelMatrix = glm::mat4(1.0f);
    viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::mat4(1.0f);
}

Gfx::~Gfx()
{
}

uint32_t Gfx::ProcUiAcquired(void* arg)
{
    Gfx* gfx = static_cast<Gfx*>(arg);
    return gfx->OnForegroundAcquired();
}

uint32_t Gfx::ProcUiReleased(void* arg)
{
    Gfx* gfx = static_cast<Gfx*>(arg);
    return gfx->OnForegroundReleased();
}

int Gfx::OnForegroundAcquired()
{
    inForeground = true;

    MEMHeapHandle fgHeap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);
    MEMHeapHandle mem1Heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);

    // Allocate tv scan buffer
    tvScanBuffer = MEMAllocFromFrmHeapEx(fgHeap, tvScanBufferSize, GX2_SCAN_BUFFER_ALIGNMENT);
    if (!tvScanBuffer) {
        return -1;
    }
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, tvScanBuffer, tvScanBufferSize);
    GX2SetTVBuffer(tvScanBuffer, tvScanBufferSize, tvRenderMode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE);

    // Allocate drc scan buffer
    drcScanBuffer = MEMAllocFromFrmHeapEx(fgHeap, drcScanBufferSize, GX2_SCAN_BUFFER_ALIGNMENT);
    if (!drcScanBuffer) {
        return -1;
    }
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, drcScanBuffer, drcScanBufferSize);
    GX2SetDRCBuffer(drcScanBuffer, drcScanBufferSize, drcRenderMode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE);

    // Allocate colorbuffers
    for (int i = 0; i < NUM_TARGETS; ++i) {
        GX2ColorBuffer& cb = colorBuffers[i];
        cb.surface.image = MEMAllocFromFrmHeapEx(mem1Heap, cb.surface.imageSize, cb.surface.alignment);
        if (!cb.surface.image) {
            return -1;
        }

        GX2Invalidate(GX2_INVALIDATE_MODE_CPU | GX2_INVALIDATE_MODE_COLOR_BUFFER, cb.surface.image, cb.surface.imageSize);
    }

    return 0;
}

int Gfx::OnForegroundReleased()
{
    MEMHeapHandle fgHeap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);
    MEMHeapHandle mem1Heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);

    // Free all foreground allocations
    MEMFreeToFrmHeap(fgHeap, MEM_FRM_HEAP_FREE_ALL);
    MEMFreeToFrmHeap(mem1Heap, MEM_FRM_HEAP_FREE_ALL);

    inForeground = false;

    return 0;
}

bool Gfx::Initialize()
{
    // Initialize GX2
    commandBufferPool = memalign(GX2_COMMAND_BUFFER_ALIGNMENT, GX2_COMMAND_BUFFER_SIZE);
    if (!commandBufferPool) {
        return false;
    }

    uint32_t initAttribs[] = {
        GX2_INIT_CMD_BUF_BASE, (uintptr_t) commandBufferPool,
        GX2_INIT_CMD_BUF_POOL_SIZE, GX2_COMMAND_BUFFER_SIZE,
        GX2_INIT_ARGC, 0,
        GX2_INIT_ARGV, 0,
        GX2_INIT_END
    };
    GX2Init(initAttribs);

    // Find the best TV render mode
    switch(GX2GetSystemTVScanMode()) {
    case GX2_TV_SCAN_MODE_480I:
    case GX2_TV_SCAN_MODE_480P:
        tvRenderMode = GX2_TV_RENDER_MODE_WIDE_480P;
        tvSize = glm::uvec2(854, 480);
        break;
    case GX2_TV_SCAN_MODE_1080I:
    case GX2_TV_SCAN_MODE_1080P:
        tvRenderMode = GX2_TV_RENDER_MODE_WIDE_1080P;
        tvSize = glm::uvec2(1920, 1080);
        break;
    case GX2_TV_SCAN_MODE_720P:
    default:
        tvRenderMode = GX2_TV_RENDER_MODE_WIDE_720P;
        tvSize = glm::uvec2(1280, 720);
        break;
    }

    // Always use double render mode as we might connect a second gamepad later
    drcRenderMode = GX2_DRC_RENDER_MODE_DOUBLE; //GX2GetSystemDRCMode();
    drcSize = glm::uvec2(854, 480);

    // Calculate TV and DRC scanbuffer size
    uint32_t unk;
    GX2CalcTVSize(tvRenderMode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE, &tvScanBufferSize, &unk);
    GX2CalcDRCSize(drcRenderMode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE, &drcScanBufferSize, &unk);

    // Initialize all 3 colorbuffers
    InitColorBuffer(colorBuffers[TARGET_TV], tvSize, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8);
    InitColorBuffer(colorBuffers[TARGET_DRC0], drcSize, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8);
    InitColorBuffer(colorBuffers[TARGET_DRC1], drcSize, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8);

    // Register callbacks for foreground allocations
    ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, ProcUiAcquired, this, 100);
    ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, ProcUiReleased, this, 100);

    // We're already in foreground at the time this is running so call the callback
    if (OnForegroundAcquired() != 0) {
        return false;
    }

    // Initialize a shared state
    contextState = (GX2ContextState*) memalign(GX2_CONTEXT_STATE_ALIGNMENT, sizeof(GX2ContextState));
    if (!contextState) {
        return false;
    }

    GX2SetupContextStateEx(contextState, TRUE);
    GX2SetContextState(contextState);

    // Set TV and DRC scale
    GX2SetTVScale(tvSize.x, tvSize.y);
    GX2SetDRCScale(drcSize.x, drcSize.y);

    // Disable depth
    GX2SetDepthOnlyControl(FALSE, FALSE, GX2_COMPARE_FUNC_ALWAYS);

    // Enable blending
    GX2SetColorControl(GX2_LOGIC_OP_COPY, 0xFF, FALSE, TRUE);

    // Setup blend control
    GX2SetBlendControl(GX2_RENDER_TARGET_0,
        GX2_BLEND_MODE_SRC_ALPHA,
        GX2_BLEND_MODE_INV_SRC_ALPHA,
        GX2_BLEND_COMBINE_MODE_ADD,
        TRUE,
        GX2_BLEND_MODE_SRC_ALPHA,
        GX2_BLEND_MODE_INV_SRC_ALPHA,
        GX2_BLEND_COMBINE_MODE_ADD);

    // Set 60fps VSync
    GX2SetSwapInterval(1);

    // Load and initialize shaders
    WHBGfxShaderGroup* colorShader = &shaderGroups[SHADER_COLOR];
    WHBGfxLoadGFDShaderGroup(colorShader, 0, colorShader_gsh);
    WHBGfxInitShaderAttribute(colorShader, "aPosition", 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    WHBGfxInitFetchShader(colorShader);

    WHBGfxShaderGroup* textureShader = &shaderGroups[SHADER_TEXTURE];
    WHBGfxLoadGFDShaderGroup(textureShader, 0, textureShader_gsh);
    WHBGfxInitShaderAttribute(textureShader, "aPosition", 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    WHBGfxInitShaderAttribute(textureShader, "aTexCoord", 0, 8, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    WHBGfxInitFetchShader(textureShader);

    // Initialize projection
    projectionMatrix = glm::ortho(0.0f, screenSpace.x, screenSpace.y, 0.0f, -1.0f, 1.0f);
    matrixUpdated = true;

    return true;
}

void Gfx::Finalize()
{
    // Release foreground if we're still in foreground
    if (inForeground) {
        OnForegroundReleased();
    }

    // Shut down GX2
    GX2Shutdown();

    // Free allocations
    free(contextState);
    contextState = nullptr;

    free(commandBufferPool);
    commandBufferPool = nullptr;

    WHBGfxFreeShaderGroup(&shaderGroups[SHADER_COLOR]);
    WHBGfxFreeShaderGroup(&shaderGroups[SHADER_TEXTURE]);
}

void Gfx::SetModel(glm::mat4& model)
{
    modelMatrix = model;
    matrixUpdated = true;
}

void Gfx::SetView(glm::mat4& view)
{
    viewMatrix = view;
    matrixUpdated = true;
}

void Gfx::BeginDraw(Target target, glm::vec4 color)
{
    currentTarget = target;
    GX2ColorBuffer* cb = &colorBuffers[target];

    // Setup colorbuffer and viewport
    GX2SetColorBuffer(cb, GX2_RENDER_TARGET_0);
    GX2SetViewport(0.0f, 0.0f, (float) cb->surface.width, (float) cb->surface.height, 0.0f, 1.0f);
    GX2SetScissor(0, 0, cb->surface.width, cb->surface.height);

    // Clear colorbuffer
    GX2ClearColor(cb, color.r, color.g, color.b, color.a);
    GX2SetContextState(contextState);
}

void Gfx::EndDraw()
{
    GX2ColorBuffer* cb = &colorBuffers[currentTarget];

    static const GX2ScanTarget scanTargets[] = {
        // TARGET_TV
        GX2_SCAN_TARGET_TV,
        // TARGET_DRC0
        GX2_SCAN_TARGET_DRC0,
        // TARGET_DRC1
        GX2_SCAN_TARGET_DRC1,
    };

    // Copy the target buffer to the scanbuffer
    GX2CopyColorBufferToScanBuffer(cb, scanTargets[currentTarget]);
    GX2SetContextState(contextState);
}

void Gfx::Draw(Texture* tex, const void* vertices, uint32_t numVertices, glm::vec4 color, bool quads)
{
    // Set wanted shader
    Shader shader = tex ? SHADER_TEXTURE : SHADER_COLOR;
    WHBGfxShaderGroup* shaderGroup = &shaderGroups[shader];
    bool shaderUpdated = false;
    if (currentShader != shader) {
        GX2SetFetchShader(&shaderGroup->fetchShader);
        GX2SetVertexShader(shaderGroup->vertexShader);
        GX2SetPixelShader(shaderGroup->pixelShader);
        currentShader = shader;
        shaderUpdated = true;
    }

    if (matrixUpdated || shaderUpdated) {
        // Calculate and set model view projection matrix
        glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
        GX2SetVertexUniformReg(shaderGroup->vertexShader->uniformVars[0].offset, 16, glm::value_ptr(mvpMatrix));
    }

    // Set color
    GX2SetPixelUniformReg(shaderGroup->pixelShader->uniformVars[0].offset, 4, glm::value_ptr(color));

    // Set up texture
    if (tex) {
        uint32_t location = shaderGroup->pixelShader->samplerVars[0].location;
        GX2SetPixelTexture(&tex->texture, location);
        GX2SetPixelSampler(&tex->sampler, location);

        GX2SetPixelUniformReg(shaderGroup->pixelShader->uniformVars[1].offset, 4, tex->texCoordParams);
    }

    // Draw
    const uint32_t stride = tex ? 16 : 8;
    GX2SetAttribBuffer(0, stride * numVertices, stride, vertices);
    GX2DrawEx(quads ? GX2_PRIMITIVE_MODE_QUADS : GX2_PRIMITIVE_MODE_TRIANGLES, numVertices, 0, 1);
}

void Gfx::SwapBuffers(void)
{
    // Swap scan buffers
    GX2SwapScanBuffers();
    GX2SetContextState(contextState);

    // Flush all packets to the GPU
    GX2Flush();

    // Enable TV and DRC on first frame rendered
    if (!displaysEnabled) {
        GX2SetTVEnable(TRUE);
        GX2SetDRCEnable(TRUE);
        displaysEnabled = true;
    }

    // Wait for flip
    uint32_t swapCount, flipCount;
    OSTime lastFlip, lastVsync;
    uint32_t waitCount = 0;
    while (true) {
        GX2GetSwapStatus(&swapCount, &flipCount, &lastFlip, &lastVsync);

        if (flipCount >= swapCount) {
            break;
        }

        if (waitCount >= 10) {
            // GPU timed out
            break;
        }

        waitCount++;
        GX2WaitForVsync();
    }
}

Gfx::Texture* Gfx::NewTexture(glm::uvec2 size, void* rgba, bool clamp, bool linearFilter)
{
    // Allocate texture
    Texture* tex = new Texture();
    if (!tex) {
        return nullptr;
    }

    // Initialize texture
    tex->texture.surface.use = GX2_SURFACE_USE_TEXTURE;
    tex->texture.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    tex->texture.surface.width = size.x;
    tex->texture.surface.height = size.y;
    tex->texture.surface.depth = 1;
    tex->texture.surface.mipLevels = 1;
    tex->texture.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
    tex->texture.surface.aa = GX2_AA_MODE1X;
    tex->texture.surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
    tex->texture.viewFirstMip = 0;
    tex->texture.viewNumMips = 1;
    tex->texture.viewFirstSlice = 0;
    tex->texture.viewNumSlices = 1;
    tex->texture.compMap = GX2_COMP_MAP(GX2_SQ_SEL_R, GX2_SQ_SEL_G, GX2_SQ_SEL_B, GX2_SQ_SEL_A);
    GX2CalcSurfaceSizeAndAlignment(&tex->texture.surface);
    GX2InitTextureRegs(&tex->texture);

    // Allocate texture surface
    tex->texture.surface.image = memalign(tex->texture.surface.alignment, tex->texture.surface.imageSize);
    if (!tex->texture.surface.image) {
        delete tex;
        return nullptr;
    }

    // Clear and invalidate texture
    memset(tex->texture.surface.image, 0, tex->texture.surface.imageSize);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, tex->texture.surface.image, tex->texture.surface.imageSize);

    // If we have any data, copy it to the texture
    if (rgba) {
        tex->Update(rgba);
    }

    // Initialize the sampler
    GX2InitSampler(&tex->sampler,
        clamp ? GX2_TEX_CLAMP_MODE_CLAMP : GX2_TEX_CLAMP_MODE_WRAP,
        linearFilter ? GX2_TEX_XY_FILTER_MODE_LINEAR : GX2_TEX_XY_FILTER_MODE_POINT);

    // No offset by default, 1x scaling
    tex->texCoordParams[0] = 0.0f;
    tex->texCoordParams[1] = 0.0f;
    tex->texCoordParams[2] = 1.0f;
    tex->texCoordParams[3] = 1.0f;

    return tex;
}

void Gfx::Texture::Update(void* rgba)
{
    uint32_t pitch = GetPitch();
    glm::uvec2 size = GetSize();
    uint8_t* dstPtr = (uint8_t*) Lock();
    uint8_t* srcPtr = (uint8_t*) rgba;

    // Copy the texture row by row
    for (uint32_t y = 0; y < size.y; ++y) {
        memcpy(dstPtr + (y * pitch * 4), srcPtr + (y * size.x * 4), size.x * 4);
    }

    Unlock();
}

uint32_t Gfx::Texture::GetPitch()
{
    return texture.surface.pitch;
}

glm::uvec2 Gfx::Texture::GetSize()
{
    return glm::uvec2(texture.surface.width, texture.surface.height);
}

void Gfx::Texture::SetClamp(bool clamp)
{
    GX2TexClampMode mode = clamp ? GX2_TEX_CLAMP_MODE_CLAMP : GX2_TEX_CLAMP_MODE_WRAP;
    GX2InitSamplerClamping(&sampler, mode, mode, mode);
}

void Gfx::Texture::SetLinearFilter(bool linear)
{
    GX2TexXYFilterMode mode = linear ? GX2_TEX_XY_FILTER_MODE_LINEAR : GX2_TEX_XY_FILTER_MODE_POINT;
    GX2InitSamplerXYFilter(&sampler, mode, mode, GX2_TEX_ANISO_RATIO_NONE);
}

void Gfx::Texture::SetUVOffset(glm::vec2 offset)
{
    texCoordParams[0] = offset.x;
    texCoordParams[1] = offset.y;
}

void Gfx::Texture::SetUVScale(glm::vec2 scale)
{
    texCoordParams[2] = scale.x;
    texCoordParams[3] = scale.y;
}

void* Gfx::Texture::Lock()
{
    return texture.surface.image;
}

void Gfx::Texture::Unlock()
{
    // Invalidate texture
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, texture.surface.image, texture.surface.imageSize);
}

void Gfx::Texture::Delete()
{
    // Free surface data and delete the texture
    free(texture.surface.image);
    delete this;
}
