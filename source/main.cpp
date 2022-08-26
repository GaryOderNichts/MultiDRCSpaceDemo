#include <whb/proc.h>

#include <coreinit/time.h>
#include <sndcore2/core.h>
#include <nsysccr/cdc.h>
#include <proc_ui/procui.h>
#include <gx2/display.h>

#include "Gfx.hpp"
#include "Text.hpp"
#include "SceneMgr.hpp"

static uint32_t OnForegroundAcquired(void* arg)
{
    // Enable multi drc to allow connecting a second gamepad
    CCRCDCSetMultiDrc(2);

    return 0;
}

static uint32_t OnForegroundReleased(void* arg)
{
    // Disconnect the second gamepad
    CCRCDCDrcState state = CCR_CDC_DRC_STATE_DISCONNECT;
    CCRCDCSysSetDrcState(CCR_CDC_DESTINATION_DRC1, &state);

    // Disable multidrc
    CCRCDCSetMultiDrc(1);

    return 0;
}

int main(int argc, char const* argv[])
{
    // Initialize ProcUI
    WHBProcInit();

    // Seed rand, used throughout the application
    srand(OSGetTick());

    // We'll need to call the CCR* functions while still in foreground so setup callbacks
    ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, OnForegroundAcquired, nullptr, 100);
    ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, OnForegroundReleased, nullptr, 100);

    // Call acquired callback since we're already in foreground
    OnForegroundAcquired(nullptr);

    // Initialize graphics
    Gfx gfx;
    gfx.Initialize();

    // Initialize AX to stop current sound from playing
    AXInit();

    // Initialize font rendering
    Text::InitializeFont();

    // Create the scene manager
    SceneMgr sceneMgr;

    while (WHBProcIsRunning()) {
        // Update scene
        sceneMgr.Update();

        // Draw TV
        gfx.BeginDraw(Gfx::TARGET_TV);
        sceneMgr.DrawScene(&gfx, Gfx::TARGET_TV);
        gfx.EndDraw();

        // Draw DRC0
        gfx.BeginDraw(Gfx::TARGET_DRC0);
        sceneMgr.DrawScene(&gfx, Gfx::TARGET_DRC0);
        gfx.EndDraw();

        // Draw DRC1
        if (GX2GetSystemDRCMode() == GX2_DRC_RENDER_MODE_DOUBLE) {
            gfx.BeginDraw(Gfx::TARGET_DRC1);
            sceneMgr.DrawScene(&gfx, Gfx::TARGET_DRC1);
            gfx.EndDraw();
        }

        // Swap buffers
        gfx.SwapBuffers();
    }

    // Deinit font rendering
    Text::DeinitializeFont();

    // Deinit AX
    AXQuit();

    // Shutdown graphics
    gfx.Finalize();

    // Call release callback
    OnForegroundReleased(nullptr);

    // Shutdown ProcUI
    WHBProcShutdown();

    return 0;
}
