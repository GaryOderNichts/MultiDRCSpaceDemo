#include "SceneMgr.hpp"

#include "Menu.hpp"
#include "Game.hpp"
#include "DrcPairing.hpp"

SceneMgr::SceneMgr()
{
    // Create scenes
    menu = new Menu(this);
    game = new Game(this);
    drcPairing = new DrcPairing(this);

    // Start with the menu scene
    currentScene = SCENE_MENU;
}

SceneMgr::~SceneMgr()
{
    delete drcPairing;
    delete game;
    delete menu;
}

void SceneMgr::SetScene(Scene scene)
{
    currentScene = scene;
}

void SceneMgr::Update()
{
    switch (currentScene) {
    case SCENE_MENU:
        menu->Update();
        break;
    case SCENE_GAME:
        game->Update();
        break;
    case SCENE_DRC_PAIRING:
        drcPairing->Update();
        break;
    }
}

void SceneMgr::DrawScene(Gfx* gfx, Gfx::Target target)
{
    switch (currentScene) {
    case SCENE_MENU:
        menu->DrawScene(gfx, target);
        break;
    case SCENE_GAME:
        game->DrawScene(gfx, target);
        break;
    case SCENE_DRC_PAIRING:
        drcPairing->DrawScene(gfx, target);
        break;
    }
}
