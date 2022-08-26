#include "Menu.hpp"
#include "SceneMgr.hpp"

#include <vpad/input.h>
#include <whb/proc.h>
#include <sysapp/launch.h>
#include <gx2/display.h>

#include "background_png.h"
#include "controls_png.h"

Menu::Menu(SceneMgr* sceneMgr) :
    sceneMgr(sceneMgr),
    frameCount(0),
    title("MultiDRCSpaceDemo", 96),
    version("Version 0.1", 32),
    menuOptions{
        Text("Start Game", 48),
        Text("Pair second GamePad", 48),
        Text("Exit", 48),
    },
    selected(0),
    confirmPromptOpened(false),
    confirmText("No second GamePad connected!", 48),
    confirmHint("Press \ue000 to start anyways, any other button to cancel", 32),
    drc1Text("Waiting for host to start game...", 48)
{
    // Load the background
    background = Sprite::FromPNG(background_png, background_png_size);
    // Fill the entire screen
    background->SetSize(Gfx::screenSpace);
    // Adjust from 1:1 to 16:9
    background->SetUVScale(glm::vec2(1.77f, 1.0f));

    // Center the title
    title.SetCentered(true);
    title.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, title.GetSize().y));

    // Setup version text
    version.SetPosition(glm::vec2(Gfx::screenSpace.x - version.GetSize().x - 8.0f,
        Gfx::screenSpace.y - version.GetSize().y));

    // Setup controls image
    controls = Sprite::FromPNG(controls_png, controls_png_size);
    controls->SetCentered(true);
    controls->SetSize(Gfx::screenSpace / 2.0f);
    controls->SetPosition(glm::vec2(Gfx::screenSpace.x / 2, (Gfx::screenSpace.y + controls->GetSize().y) / 2));

    // Initialize option positions
    float yOffset = Gfx::screenSpace.y / 2.0f;
    for (size_t i = 0; i < COUNTOF(menuOptions); ++i) {
        menuOptions[i].SetCentered(true);
        menuOptions[i].SetPosition(glm::vec2(Gfx::screenSpace.x / 2, yOffset));
        yOffset += menuOptions[i].GetSize().y;
    }

    // Highlight the initially selected option
    menuOptions[selected].SetColor(glm::vec4(0.75f, 0.33f, 0.92f, 1.0f));

    // Initialize confirm prompt
    confirmText.SetPosition(Gfx::screenSpace / 2.0f);
    confirmText.SetCentered(true);
    confirmHint.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, Gfx::screenSpace.y - confirmHint.GetSize().y));
    confirmHint.SetCentered(true);

    // Set drc1 message position
    drc1Text.SetCentered(true);
    drc1Text.SetPosition(Gfx::screenSpace / 2.0f);
}

Menu::~Menu()
{
    delete controls;
    delete background;
}

void Menu::Update()
{
    frameCount++;

    // Animate the background
    background->SetUVOffset(glm::vec2(frameCount * 0.005f, frameCount * -0.005f));

    // Animate the title
    title.SetAngle(sin(frameCount * 4.0f * M_PI / 180.0f) * 8.0f);
    title.SetScale(glm::vec2(SCALE(sin(frameCount * 10.0f * M_PI / 180.0f), -1.0f, 1.0f, 0.9f, 1.0f)));

    // Menu can only be controlled by the host (Gamepad 0)
    VPADStatus status{};
    VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

    // Handle the confirm prompt
    if (confirmPromptOpened) {
        if (status.trigger & VPAD_BUTTON_A) {
            sceneMgr->SetScene(SceneMgr::SCENE_GAME);
        }

        if (status.trigger) {
            confirmPromptOpened = false;
        }

        return;
    }

    // Update menu selection
    if (status.trigger & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN)) {
        if (selected <= COUNTOF(menuOptions) - 2) {
            selected++;
        }
    } else if (status.trigger & (VPAD_BUTTON_UP | VPAD_STICK_L_EMULATION_UP)) {
        if (selected > 0) {
            selected--;
        }
    }

    // Handle selection
    if (status.trigger & (VPAD_BUTTON_A | VPAD_BUTTON_PLUS)) {
        switch (selected) {
        case 0:
            if (GX2GetSystemDRCMode() == GX2_DRC_RENDER_MODE_DOUBLE) {
                sceneMgr->SetScene(SceneMgr::SCENE_GAME);
            } else {
                confirmPromptOpened = true;
            }
            break;
        case 1:
            sceneMgr->SetScene(SceneMgr::SCENE_DRC_PAIRING);
            break;
        case 2:
            // If we're not running from the HBL launch the Wii U menu
            if (!RunningFromHBL()) {
                SYSLaunchMenu();
            } else {
                WHBProcStopRunning();
            }
            break;
        }
    }

    // Highlight the selected entry
    for (size_t i = 0; i < COUNTOF(menuOptions); ++i) {
        menuOptions[i].SetColor(selected == i ? glm::vec4(0.75f, 0.33f, 0.92f, 1.0f) : glm::vec4(1.0f));
    }
}

void Menu::DrawScene(Gfx* gfx, Gfx::Target target)
{
    // Default view (identity matrix)
    glm::mat4 view = glm::mat4(1.0f);
    gfx->SetView(view);

    background->Draw(gfx);
    title.Draw(gfx);
    version.Draw(gfx);

    // Draw target specific elements
    if (target == Gfx::TARGET_TV) {
        controls->Draw(gfx);
    } else if (target == Gfx::TARGET_DRC0) {
        if (confirmPromptOpened) {
            confirmText.Draw(gfx);
            confirmHint.Draw(gfx);
        } else {
            for (size_t i = 0; i < COUNTOF(menuOptions); ++i) {
                menuOptions[i].Draw(gfx);
            }
        }
    } else if (target == Gfx::TARGET_DRC1) {
        drc1Text.Draw(gfx);
    }
}
