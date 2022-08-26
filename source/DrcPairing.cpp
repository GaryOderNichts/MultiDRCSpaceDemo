#include "DrcPairing.hpp"
#include "SceneMgr.hpp"

#include <malloc.h>

#include <vpad/input.h>
#include <nn/ccr.h>
#include <nsysccr/cdc.h>

#include "background_png.h"

// 2 minutes timeout
#define TIMEOUT_SECONDS 120

DrcPairing::DrcPairing(SceneMgr* sceneMgr) :
    sceneMgr(sceneMgr),
    frameCount(0),
    titleText("Pairing second GamePad", 96),
    pinText("Pin: ---- ", 48),
    timeoutText("000 seconds remaining", 48),
    doneText("Paired second GamePad", 48),
    errorText("Failed to pair GamePad", 48),
    hintText("Press the console's SYNC button to cancel", 32),
    hintText2("Press any button to continue", 32),
    state(STATE_START)
{
    // Load the background
    background = Sprite::FromPNG(background_png, background_png_size);
    // Fill the entire screen
    background->SetSize(Gfx::screenSpace);
    // Adjust from 1:1 to 16:9
    background->SetUVScale(glm::vec2(1.77f, 1.0f));

    // Setup texts
    titleText.SetCentered(true);
    titleText.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, titleText.GetSize().y));
    pinText.SetCentered(true);
    pinText.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, (Gfx::screenSpace.y / 2) - pinText.GetSize().y));
    timeoutText.SetCentered(true);
    timeoutText.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, (Gfx::screenSpace.y / 2) + timeoutText.GetSize().y));
    doneText.SetCentered(true);
    doneText.SetPosition(Gfx::screenSpace / 2.0f);
    errorText.SetCentered(true);
    errorText.SetPosition(Gfx::screenSpace / 2.0f);
    hintText.SetCentered(true);
    hintText.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, Gfx::screenSpace.y - hintText.GetSize().y));
    hintText2.SetCentered(true);
    hintText2.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, Gfx::screenSpace.y - hintText2.GetSize().y));

    // Initialize IM
    imHandle = IM_Open();
    imRequest = (IMRequest*) memalign(0x40, sizeof(IMRequest));
    // Allocate a separate request for IM_CancelGetEventNotify to avoid conflict with the pending IM_GetEventNotify request
    imCancelRequest = (IMRequest*) memalign(0x40, sizeof(IMRequest));

    // Init CCRSys
    CCRSysInit();
}

DrcPairing::~DrcPairing()
{
    // Deinit CCRSys
    CCRSysExit();

    // Close IM
    IM_CancelGetEventNotify(imHandle, imCancelRequest, nullptr, nullptr);
    IM_Close(imHandle);
    free(imCancelRequest);
    free(imRequest);

    delete background;
}

void DrcPairing::Update()
{
    frameCount++;

    // Animate the background
    background->SetUVOffset(glm::vec2(frameCount * 0.005f, frameCount * -0.005f));

    switch (state) {
    case STATE_START: {
        // Setup sync callback to allow cancelling with the SYNC button
        cancelPairing = false;
        // Set mask to only sync events
        imEventMask = IM_EVENT_SYNC;
        // Notify about sync button events
        IM_GetEventNotify(imHandle, imRequest, &imEventMask, DrcPairing::SyncButtonCallback, &imEventMask);

        // Get the pincode
        uint32_t pincode;
        if (CCRSysGetPincode(&pincode) != 0) {
            state = STATE_ERROR;
            return;
        }

        // Convert the pin to symbols and set the text
        static char pinSymbols[][4] = {
            "\u2660",
            "\u2665",
            "\u2666",
            "\u2663"
        };
        std::string pin = std::string(pinSymbols[(pincode / 1000) % 10]) + 
            pinSymbols[(pincode / 100) % 10] + 
            pinSymbols[(pincode / 10) % 10] +
            pinSymbols[pincode % 10];
        pinText.SetText("Pin: " + pin);

        // Start pairing to slot 1 (second gamepad)
        if (CCRSysStartPairing(1, TIMEOUT_SECONDS) != 0) {
            state = STATE_ERROR;
            return;
        }

        // Pairing has started, save start time
        startTime = OSGetTime();
        state = STATE_PAIRING;
        break;
    }
    case STATE_PAIRING: {
        // Get the current pairing state
        CCRSysPairingState pairingState = CCRSysGetPairingState();
        if (pairingState == CCR_SYS_PAIRING_TIMED_OUT || cancelPairing) {
            // Pairing has timed out or was cancelled
            CCRSysStopPairing();
            state = STATE_ERROR;
            return;
        } else if (pairingState == CCR_SYS_PAIRING_FINISHED) {
            // Second gamepad was paired
            state = STATE_DONE;
            return;
        }

        // Update timeout text
        timeoutText.SetText(std::to_string(TIMEOUT_SECONDS - OSTicksToSeconds(OSGetTime() - startTime)) + " seconds remaining");
        break;
    }
    case STATE_ERROR:
    case STATE_DONE: {
        VPADStatus status{};
        VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

        if (status.trigger) {
            // Reset state
            state = STATE_START;

            // Reset sync callback
            cancelPairing = false;
            IM_CancelGetEventNotify(imHandle, imCancelRequest, nullptr, nullptr);

            // Return to menu
            sceneMgr->SetScene(SceneMgr::SCENE_MENU);
            return;
        }
        break;
    }
    default:
        break;
    }
}

void DrcPairing::DrawScene(Gfx* gfx, Gfx::Target target)
{
    // Default view (identity matrix)
    glm::mat4 view = glm::mat4(1.0f);
    gfx->SetView(view);

    background->Draw(gfx);

    titleText.Draw(gfx);

    switch (state) {
    case STATE_START:
        break;
    case STATE_PAIRING:
        pinText.Draw(gfx);
        timeoutText.Draw(gfx);
        hintText.Draw(gfx);
        break;
    case STATE_ERROR:
        errorText.Draw(gfx);
        if (target == Gfx::TARGET_DRC0) {
            hintText2.Draw(gfx);
        }
        break;
    case STATE_DONE:
        doneText.Draw(gfx);
        if (target == Gfx::TARGET_DRC0) {
            hintText2.Draw(gfx);
        }
        break;
    }
}

void DrcPairing::SyncButtonCallback(IOSError error, void* arg)
{
    uint32_t event = *(uint32_t*) arg;

    // Cancel pairing if the sync button was pressed
    if (error == IOS_ERROR_OK && (event & IM_EVENT_SYNC)) {
        cancelPairing = true;
    }
}
