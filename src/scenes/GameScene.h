#ifndef SCENES_GAME_SCENE_H
#define SCENES_GAME_SCENE_H

#include <SDL3_mixer/SDL_mixer.h>
#include <vector>

#include "core/scene/Scene.h"
#include "game/Mode.h"
#include "game/Components.h"
#include <RmlUi/Core/ElementDocument.h>

class GameScene : public core::scene::Scene
{
public:
    explicit GameScene(AppContext *context, game::mode::Mode gameMode);
    GameScene() = delete;
    ~GameScene() override;

    // Lifecycle
    bool Init() override;
    void Ready() override;
    void OnEnter() override;
    void OnExit() override;
    void CleanUp() override;

    // Main loop
    SDL_AppResult HandleEvent(SDL_Event *event) override;
    void Update(float deltaTime) override;
    void Render() override;

    void onSecondCounterTimer();

private:
    // Game constants
    game::mode::Mode gameMode;

    int initialSpeed = 360;

    // Game variables
    int scores[2]{0, 0}; // Scores for player 1 and player 2
    int gameTime{};
    int soloScore{};
    int multiplier{};
    int winning_points{};
    SDL_TimerID secondCounterTimer;
    float timeAfterGameEnded{-1.0f};

    // RmlUi
    Rml::ElementDocument *doc{nullptr};

    // Components
    struct Ball
    {
        Radius radius;
        Velocity velocity;
        Speed speed;
        SDL_Texture *sprite{nullptr};
        SDL_FRect rec;
    } ball;

    struct Paddle
    {
        SDL_FRect rec;
        Speed speed;
        int direction{}; // 0, 1 or -1
    };

    SDL_Texture *paddleSprite{nullptr};
    Paddle paddles[2];          // Paddles for players
    Size2D lastKnownRenderSize; // To compare on resize

    // SDL resources
    SDL_Texture *scoreTexture{nullptr};
    MIX_Audio *wallBounceSound{nullptr};
    MIX_Audio *paddleBounceSound{nullptr};
    MIX_Audio *scoreSound{nullptr};
    MIX_Track *wallBounceTrack{nullptr};
    MIX_Track *paddleBounceTrack{nullptr};
    MIX_Track *scoreTrack{nullptr};

    // Helper functions
    void ResetBall();
    void UpdatePaddleMovement(int paddleIndex, int direction, float deltaTime);
    void CheckCollisions();
    SDL_Texture *LoadImageTexture(const std::string &path);
    void adjustToScreen();
    void UpdateScore(int scorerIndex);
    void UpdateScoreDisplay();
    void CheckGameOver();
};

#endif // SCENES_GAME_SCENE_H
