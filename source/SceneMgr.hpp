#pragma once

#include "Gfx.hpp"

class SceneMgr {
public:
    enum Scene {
        SCENE_MENU,
        SCENE_GAME,
        SCENE_DRC_PAIRING,
    };

    SceneMgr();
    virtual ~SceneMgr();

    void SetScene(Scene scene);

    void Update();

    void DrawScene(Gfx* gfx, Gfx::Target target);

protected:
    Scene currentScene;

    class Menu* menu;
    class Game* game;
    class DrcPairing* drcPairing;
};
