#pragma once

#include "Gfx.hpp"
#include "Sprite.hpp"
#include "Text.hpp"

#include <vector>

class SceneMgr;

class Game {
public:
    Game(SceneMgr* sceneMgr);
    virtual ~Game();

    void Update();

    void DrawScene(Gfx* gfx, Gfx::Target target);

    void Reset();

    void PauseGame(bool pause);

private:
    SceneMgr* sceneMgr;

    uint32_t frameCount;

    bool paused;
    Sprite pauseBackground;
    Text pauseText;
    Text pauseHint;

    bool gameOver;
    int winner;
    Text gameOverText;
    Text gameOverSubText;

    Sprite* tvBackground;
    Sprite* mapBackground;
    Sprite* mapPlayers[2];
    Text tvPlayerLives[2];

    Sprite* background;
    Sprite* borders[4];

    struct Bullet {
        Sprite sprite;
        glm::vec2 velocity;
        uint32_t timeLeft;
    };

    struct Particle {
        Sprite sprite;
        glm::vec2 velocity;
        uint32_t timeLeft;
    };

    struct Player {
        Game* game;
        int playerNum;
        uint32_t lives;
        Text livesText;
        uint32_t shootTimeout;

        Sprite* sprite;
        glm::vec2 velocity;

        std::vector<Bullet> bullets;
        std::vector<Particle> particles;

        Player(Game* game, int playerNum);
        virtual ~Player();

        void Update();
    };

    Player* players[2];
};
