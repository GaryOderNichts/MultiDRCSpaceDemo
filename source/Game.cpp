#include "Game.hpp"
#include "SceneMgr.hpp"

#include <vpad/input.h>

#include "background_png.h"
#include "border_png.h"
#include "spaceship_small_red_png.h"
#include "spaceship_small_blue_png.h"

#define FIELD_WIDTH (1024.0f * 3)
#define FIELD_HEIGHT (1024.0f * 3)
#define BORDER_SIZE 1024.0f

#define NUM_LIVES 5
#define HEART_FULL "\ue017" // "\u2665"
#define HEART_EMPTY "\ue01f" // "\u2661"

Game::Game(SceneMgr* sceneMgr) :
    sceneMgr(sceneMgr),
    frameCount(0),
    paused(false),
    pauseBackground(glm::vec2(0.0f), Gfx::screenSpace, 0.0f, glm::vec4(0.5f)),
    pauseText("Game is paused", 64),
    pauseHint("      \ue000/\ue045 Continue         \ue001 Return to menu", 32),
    gameOverText("Game Over", 96),
    gameOverSubText("Player X won!", 64),
    tvPlayerLives{
        Text("-", 64, glm::vec2(0.0f), glm::vec2(1.0f), 0.0f, glm::vec4(0.89f, 0.0f, 0.0f, 1.0f)),
        Text("-", 64, glm::vec2(0.0f), glm::vec2(1.0f), 0.0f, glm::vec4(0.2f, 0.62f, 0.8f, 1.0f)),
    }
{
    // Initialize pause items
    pauseBackground.SetVisible(false);
    pauseText.SetPosition(Gfx::screenSpace / 2.0f);
    pauseText.SetCentered(true);
    pauseText.SetVisible(false);
    pauseHint.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, Gfx::screenSpace.y - pauseHint.GetSize().y));
    pauseHint.SetCentered(true);
    pauseHint.SetVisible(false);

    // Initialize game over screens
    gameOverText.SetPosition(glm::vec2(Gfx::screenSpace.x / 2, pauseHint.GetSize().y));
    gameOverText.SetCentered(true);
    gameOverText.SetVisible(false);
    gameOverSubText.SetPosition(Gfx::screenSpace / 2.0f);
    gameOverSubText.SetCentered(true);
    gameOverSubText.SetVisible(false);

    // Initialize map items
    mapBackground = Sprite::FromPNG(background_png, background_png_size);
    mapBackground->SetCentered(true);
    mapBackground->SetSize(glm::vec2(512.0f));
    mapBackground->SetPosition(Gfx::screenSpace / 2.0f);
    mapPlayers[0] = Sprite::FromPNG(spaceship_small_red_png, spaceship_small_red_png_size);
    mapPlayers[0]->SetCentered(true);
    mapPlayers[0]->SetScale(glm::vec2(0.5f));
    mapPlayers[1] = Sprite::FromPNG(spaceship_small_blue_png, spaceship_small_blue_png_size);
    mapPlayers[1]->SetCentered(true);
    mapPlayers[1]->SetScale(glm::vec2(0.5f));

    // Initialize player lives
    tvPlayerLives[0].SetPosition(glm::vec2(8.0f, Gfx::screenSpace.y - tvPlayerLives[0].GetSize().y));

    // Need to initialize right side text to calculate width
    std::string text;
    for (int i = 0; i < NUM_LIVES; ++i) {
        text += HEART_FULL;
    }
    tvPlayerLives[1].SetText(text);
    tvPlayerLives[1].SetPosition(glm::vec2(Gfx::screenSpace.x - tvPlayerLives[1].GetSize().x - 8.0f,
        Gfx::screenSpace.y - tvPlayerLives[1].GetSize().y));

    // Load the backgrounds
    background = Sprite::FromPNG(background_png, background_png_size);
    tvBackground = Sprite::FromPNG(border_png, border_png_size);
    // Fill the entire screen
    background->SetSize(Gfx::screenSpace);
    tvBackground->SetSize(Gfx::screenSpace);
    // Adjust from 1:1 to 16:9
    background->SetUVScale(glm::vec2(1.77f, 1.0f));
    tvBackground->SetUVScale(glm::vec2(1.77f, 1.0f));

    // Create players
    players[0] = new Player(this, 0);
    players[1] = new Player(this, 1);

    // Create borders
    // Top
    borders[0] = Sprite::FromPNG(border_png, border_png_size);
    borders[0]->SetPosition(glm::vec2(0.0f, -((FIELD_HEIGHT + BORDER_SIZE) / 2)));
    borders[0]->SetSize(glm::vec2(FIELD_WIDTH + BORDER_SIZE, BORDER_SIZE));
    borders[0]->SetCentered(true);
    borders[0]->SetUVScale(glm::vec2(borders[0]->GetSize().x / 1024.0f, borders[0]->GetSize().y / 1024.0f));
    // Bottom
    borders[1] = Sprite::FromPNG(border_png, border_png_size);
    borders[1]->SetPosition(glm::vec2(0.0f, (FIELD_HEIGHT + BORDER_SIZE) / 2));
    borders[1]->SetSize(glm::vec2(FIELD_WIDTH + BORDER_SIZE, BORDER_SIZE));
    borders[1]->SetCentered(true);
    borders[1]->SetUVScale(glm::vec2(borders[1]->GetSize().x / 1024.0f, borders[1]->GetSize().y / 1024.0f));
    // Left
    borders[2] = Sprite::FromPNG(border_png, border_png_size);
    borders[2]->SetPosition(glm::vec2(-((FIELD_WIDTH + BORDER_SIZE) / 2), 0.0f));
    borders[2]->SetSize(glm::vec2(BORDER_SIZE, FIELD_HEIGHT + BORDER_SIZE));
    borders[2]->SetCentered(true);
    borders[2]->SetUVScale(glm::vec2(borders[2]->GetSize().x / 1024.0f, borders[2]->GetSize().y / 1024.0f));
    // Right
    borders[3] = Sprite::FromPNG(border_png, border_png_size);
    borders[3]->SetPosition(glm::vec2((FIELD_WIDTH + BORDER_SIZE) / 2, 0.0f));
    borders[3]->SetSize(glm::vec2(BORDER_SIZE, FIELD_HEIGHT + BORDER_SIZE));
    borders[3]->SetCentered(true);
    borders[3]->SetUVScale(glm::vec2(borders[3]->GetSize().x / 1024.0f, borders[3]->GetSize().y / 1024.0f));

    // Initialize game
    Reset();
}

Game::~Game()
{
    for (Sprite* s : borders) {
        delete s;
    }

    for (Sprite* s : mapPlayers) {
        delete s;
    }

    delete players[0];
    delete players[1];
    delete tvBackground;
    delete mapBackground;
    delete background;
}

void Game::Update()
{
    // Handle paused state
    if (paused || gameOver) {
        VPADStatus status{};
        VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

        // Continue
        if (status.trigger & (VPAD_BUTTON_A | VPAD_BUTTON_PLUS)) {
            if (gameOver) {
                Reset();
            } else {
                PauseGame(false);
            }
        }

        // Return to menu
        if (status.trigger & VPAD_BUTTON_B) {
            // Reset game first
            Reset();
            sceneMgr->SetScene(SceneMgr::SCENE_MENU);
        }

        return;
    }

    frameCount++;

    // Update players
    for (Player* p : players) {
        p->Update();

        // Check for collision against the other players bullets
        for (Bullet& b : players[p->playerNum ? 0 : 1]->bullets) {
            // Use a simple distance check, not great but will do for this demo
            if (glm::length(b.sprite.GetPosition() - p->sprite->GetPosition()) < 32.0f) {
                // Spawn explosion particles
                for (int i = 0; i < 100; ++i) {
                    p->particles.push_back(Particle{
                        Sprite(
                            // Position: Create particles around player
                            p->sprite->GetPosition() + glm::vec2(frand(-2.5f, 2.5f), frand(-2.5f, 2.5f)),
                            // Size
                            glm::vec2(2.0f),
                            // Angle: Random rotations
                            frand(-90.0f, 90.0f),
                            // Color: Random reddish colors
                            glm::vec4(frand(0.5f, 1.0f), 0.0f, 0.0f, 1.0f)
                        ),
                        // Velocity: random velocity
                        glm::vec2(frand(-1.0f, 1.0f), frand(-1.0f, 1.0f)),
                        // Random time-to-live
                        20u + (rand() % 20)
                    });
                }

                // Decrease lives
                p->lives--;
                std::string text;
                for (uint32_t i = 0; i < NUM_LIVES; ++i) {
                    if (p->lives > i) {
                        text += HEART_FULL;
                    } else {
                        text += HEART_EMPTY;
                    }
                }
                p->livesText.SetText(text);
                tvPlayerLives[p->playerNum].SetText(text);

                // Handle game over
                if (p->lives == 0) {
                    // Hide the player
                    p->sprite->SetVisible(false);
                    
                    gameOver = true;
                    winner = !p->playerNum;
                    // Show pause and game over texts
                    pauseBackground.SetVisible(true);
                    pauseHint.SetVisible(true);
                    gameOverText.SetVisible(true);
                    gameOverSubText.SetVisible(true);

                    // Update text and color to match winner
                    gameOverSubText.SetText(winner ? "Player 2 won!" : "Player 1 won!");
                    gameOverSubText.SetColor(winner ? glm::vec4(0.2f, 0.62f, 0.8f, 1.0f) : glm::vec4(0.89f, 0.0f, 0.0f, 1.0f));
                    return;
                }

                // Place player in a random position
                p->sprite->SetPosition(glm::vec2(
                    frand(-(FIELD_WIDTH / 2), (FIELD_WIDTH / 2)),
                    frand(-(FIELD_HEIGHT / 2), (FIELD_HEIGHT / 2))));
            }
        }
    }

    // Animate the TV background
    tvBackground->SetUVOffset(glm::vec2(frameCount * 0.005f, frameCount * -0.005f));

    // Update map player icons
    mapPlayers[0]->SetPosition((Gfx::screenSpace / 2.0f) + (glm::vec2(
        players[0]->sprite->GetPosition().x / FIELD_WIDTH, players[0]->sprite->GetPosition().y / FIELD_HEIGHT) * 512.0f));
    mapPlayers[0]->SetAngle(players[0]->sprite->GetAngle());
    mapPlayers[1]->SetPosition((Gfx::screenSpace / 2.0f) + (glm::vec2(
        players[1]->sprite->GetPosition().x / FIELD_WIDTH, players[1]->sprite->GetPosition().y / FIELD_HEIGHT) * 512.0f));
    mapPlayers[1]->SetAngle(players[1]->sprite->GetAngle());
}

void Game::DrawScene(Gfx* gfx, Gfx::Target target)
{
    if (target == Gfx::TARGET_TV) {
        // Default view (identity matrix)
        glm::mat4 view = glm::mat4(1.0f);
        gfx->SetView(view);

        // Draw the background
        tvBackground->Draw(gfx);

        // Draw the map
        mapBackground->Draw(gfx);
        mapPlayers[0]->Draw(gfx);
        mapPlayers[1]->Draw(gfx);

        // Draw player lives
        tvPlayerLives[0].Draw(gfx);
        tvPlayerLives[1].Draw(gfx);
    } else {
        Player* targetPlayer = players[target - 1];

        // Default view (identity matrix)
        glm::mat4 view = glm::mat4(1.0f);
        gfx->SetView(view);

        // Offset the background based on player position
        background->SetUVOffset(targetPlayer->sprite->GetPosition() / background->GetScaledSize());
        background->Draw(gfx);

        // Setup a centered camera which follows the player
        glm::vec3 cameraPosition = glm::vec3(targetPlayer->sprite->GetPosition() - Gfx::screenSpace / 2.0f, 0.0f);
        view = glm::lookAt(cameraPosition, cameraPosition + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        gfx->SetView(view);

        // Draw borders
        for (Sprite* s : borders) {
            s->Draw(gfx);
        }

        // Draw particles and bullets first so they don't overlap players
        for (Player* p : players) {
            for (Particle& part : p->particles) {
                part.sprite.Draw(gfx);
            }
            for (Bullet& b : p->bullets) {
                b.sprite.Draw(gfx);
            }
        }

        // Draw player sprites
        for (Player* p : players) {
            p->sprite->Draw(gfx);
        }
    }

    // Default view (identity matrix)
    glm::mat4 view = glm::mat4(1.0f);
    gfx->SetView(view);

    if (target == Gfx::TARGET_DRC0) {
        players[0]->livesText.Draw(gfx);
    } else if (target == Gfx::TARGET_DRC1) {
        players[1]->livesText.Draw(gfx);
    }

    pauseBackground.Draw(gfx);
    pauseText.Draw(gfx);
    gameOverText.Draw(gfx);
    gameOverSubText.Draw(gfx);

    if (target == Gfx::TARGET_DRC0) {
        pauseHint.Draw(gfx);
    }
}

void Game::Reset()
{
    for (Player* p : players) {
        p->bullets.clear();
        p->particles.clear();
        p->lives = NUM_LIVES;
        p->shootTimeout = 0;
        p->velocity = glm::vec2(0.0f);
        p->sprite->SetAngle(0.0f);
        p->sprite->SetVisible(true);

        std::string text;
        for (int i = 0; i < NUM_LIVES; ++i) {
            text += HEART_FULL;
        }
        p->livesText.SetText(text);
        tvPlayerLives[p->playerNum].SetText(text);
    }

    // Place players in a random position
    players[0]->sprite->SetPosition(glm::vec2(
        frand(-(FIELD_WIDTH / 2), (FIELD_WIDTH / 2)),
        frand(-(FIELD_HEIGHT / 2), (FIELD_HEIGHT / 2))));
    players[1]->sprite->SetPosition(glm::vec2(
        frand(-(FIELD_WIDTH / 2), (FIELD_WIDTH / 2)),
        frand(-(FIELD_HEIGHT / 2), (FIELD_HEIGHT / 2))));

    gameOver = false;
    gameOverText.SetVisible(false);
    gameOverSubText.SetVisible(false);

    PauseGame(false);
}

void Game::PauseGame(bool pause)
{
    pauseBackground.SetVisible(pause);
    pauseText.SetVisible(pause);
    pauseHint.SetVisible(pause);

    paused = pause;
}

Game::Player::Player(Game* game, int playerNum) :
    game(game),
    playerNum(playerNum),
    lives(NUM_LIVES),
    livesText("-", 64, glm::vec2(0.0f), glm::vec2(1.0f), 0.0f,
        playerNum ? glm::vec4(0.2f, 0.62f, 0.8f, 1.0f) : glm::vec4(0.89f, 0.0f, 0.0f, 1.0f)),
    shootTimeout(0),
    velocity(glm::vec2(0.0f))
{
    // Load player sprite
    sprite = Sprite::FromPNG(
            playerNum ? spaceship_small_blue_png : spaceship_small_red_png,
            playerNum ? spaceship_small_blue_png_size : spaceship_small_red_png_size);
    sprite->SetCentered(true);
    sprite->SetScale(glm::vec2(1.5f));
    // Disable filtering for pixel art
    sprite->SetLinearFilter(false);

    // Initialize lives text
    livesText.SetPosition(glm::vec2(8.0f, Gfx::screenSpace.y - livesText.GetSize().y));
}

Game::Player::~Player()
{
    delete sprite;
}

void Game::Player::Update()
{
    VPADStatus status{};
    VPADRead((VPADChan) playerNum, &status, 1, nullptr);

    // Pause (only allowed by host)
    if (playerNum == 0 && (status.trigger & VPAD_BUTTON_PLUS)) {
        game->PauseGame(true);
    }

    // Shoot
    if (status.hold & (VPAD_BUTTON_ZR | VPAD_BUTTON_R)) {
        if (shootTimeout == 0) {
            bullets.push_back(Bullet{
                Sprite(
                    // Position: Spawn bullet at player position
                    sprite->GetPosition(),
                    // Size
                    glm::vec2(4.0f, 10.0f),
                    // Use player angle for rotation
                    sprite->GetAngle(),
                    // Color
                    glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)),
                // Bullet velocity based on direction and current player velocity
                sprite->GetForwardVector() * (16.0f + glm::length(velocity)),
                // time to live
                600
            });

            // Set timeout
            shootTimeout = 5;
        }
    }

    // Update shoot timeout
    if (shootTimeout > 0) {
        shootTimeout--;
    }

    glm::vec2 leftStick = glm::vec2(status.leftStick.x, -status.leftStick.y);
    glm::vec2 rightStick = glm::vec2(status.rightStick.x, -status.rightStick.y);

    // Move
    if (glm::length(leftStick) > 0.1f) {
        float speed = 8.0f;

        // Speed boost
        // TODO maybe add a cooldown?
        if (status.hold & (VPAD_BUTTON_ZL | VPAD_BUTTON_L)) {
            // Double speed while boosting
            speed *= 2.0f;

            // Spawn boost particles
            for (int i = 0; i < 30; ++i) {
                particles.push_back(Particle{
                    Sprite(
                        // Position: Create trail of particles behind player
                        sprite->GetPosition() + (leftStick * glm::vec2(-(i % 15))),
                        // Size
                        glm::vec2(2.0f),
                        // Angle: Random rotations
                        frand(-90.0f, 90.0f),
                        // Color: Random reddish colors
                        glm::vec4(frand(0.5f, 1.0f) + 0.5f, 0.0f, 0.0f, 1.0f)
                    ),
                    // Velocity: Move in opposite player position with random offsets
                    -sprite->GetForwardVector() + glm::vec2(frand(), frand()),
                    // Random time-to-live
                    40u + (rand() % 40)
                });
            }
        }

        // Set velocity based on stick and speed
        velocity = leftStick * speed;
        // Rotate player in move direction
        sprite->SetAngle(glm::degrees(atan2(leftStick.x, -leftStick.y)));
    }

    // Check for border collision
    if ((sprite->GetPosition().x + velocity.x) > (FIELD_WIDTH / 2) || (sprite->GetPosition().x + velocity.x) < -(FIELD_WIDTH / 2)) {
        velocity.x = -velocity.x * 3.0f;
    }
    if ((sprite->GetPosition().y + velocity.y) > (FIELD_HEIGHT / 2) || (sprite->GetPosition().y + velocity.y) < -(FIELD_HEIGHT / 2)) {
        velocity.y = -velocity.y * 3.0f;
    }

    // Update position
    sprite->SetPosition(sprite->GetPosition() + velocity);

    // Update velocity: Slowly decreasing
    velocity *= glm::vec2(0.95f);
    if (glm::length(velocity) < 0.002f) {
        velocity = glm::vec2(0.0f);
    }

    // Right stick overrides rotation
    if (glm::length(rightStick) > 0.1f) {
        sprite->SetAngle(glm::degrees(atan2(rightStick.x, -rightStick.y)));
    }

    // Update particles
    for (size_t i = 0; i < particles.size(); ++i) {
        Particle& p = particles[i];
        if (--p.timeLeft == 0) {
            particles.erase(particles.begin() + i);
            continue;;
        }

        p.sprite.SetPosition(p.sprite.GetPosition() + p.velocity);
    }

    // Update bullets
    for (size_t i = 0; i < bullets.size(); ++i) {
        Bullet& b = bullets[i];
        if (--b.timeLeft == 0) {
            bullets.erase(bullets.begin() + i);
            continue;;
        }

        b.sprite.SetPosition(b.sprite.GetPosition() + b.velocity);
    }
}
