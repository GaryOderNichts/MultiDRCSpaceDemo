#pragma once

#include "Gfx.hpp"
#include "Text.hpp"

class SceneMgr;

class Menu {
public:
    Menu(SceneMgr* sceneMgr);
    virtual ~Menu();

    void Update();

    void DrawScene(Gfx* gfx, Gfx::Target target);

private:
    SceneMgr* sceneMgr;
    uint32_t frameCount;

    Sprite* background;
    Text title;
    Text version;
    Sprite* controls;

    Text menuOptions[3];
    size_t selected;

    bool confirmPromptOpened;
    Text confirmText;
    Text confirmHint;

    Text drc1Text;
    Sprite* controlsImage;
};
