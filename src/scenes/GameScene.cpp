#include "GameScene.h"
#include "core/scene/Events.h"

#include <SDL3_image/SDL_image.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core.h>
#include <format>

GameScene::GameScene(AppContext *context, game::mode::Mode mode) : Scene("Game", context), gameMode(mode)
{
}

GameScene::~GameScene()
{
    CleanUp();
}

bool GameScene::Init()
{
    if (app->mixer)
    {
        wallBounceSound = MIX_LoadAudio(app->mixer, "assets/sounds/ping.wav", false);
        paddleBounceSound = MIX_LoadAudio(app->mixer, "assets/sounds/pong.wav", false);
        scoreSound = MIX_LoadAudio(app->mixer, "assets/sounds/score.wav", false);
        wallBounceTrack = MIX_CreateTrack(app->mixer);
        paddleBounceTrack = MIX_CreateTrack(app->mixer);
        scoreTrack = MIX_CreateTrack(app->mixer);
        if (wallBounceTrack && wallBounceSound)
        {
            MIX_SetTrackAudio(wallBounceTrack, wallBounceSound);
        }
        if (paddleBounceTrack && paddleBounceSound)
        {
            MIX_SetTrackAudio(paddleBounceTrack, paddleBounceSound);
        }
        if (scoreTrack && scoreSound)
        {
            MIX_SetTrackAudio(scoreTrack, scoreSound);
        }
    }

    ball.sprite = LoadImageTexture("assets/ball.png");
    paddleSprite = LoadImageTexture("assets/paddle.png");

    return ball.sprite && paddleSprite;
}

void GameScene::CleanUp()
{
    if (wallBounceTrack)
    {
        MIX_DestroyTrack(wallBounceTrack);
        wallBounceTrack = nullptr;
    }
    if (paddleBounceTrack)
    {
        MIX_DestroyTrack(paddleBounceTrack);
        paddleBounceTrack = nullptr;
    }
    if (scoreTrack)
    {
        MIX_DestroyTrack(scoreTrack);
        scoreTrack = nullptr;
    }
    if (wallBounceSound)
    {
        MIX_DestroyAudio(wallBounceSound);
        wallBounceSound = nullptr;
    }
    if (paddleBounceSound)
    {
        MIX_DestroyAudio(paddleBounceSound);
        paddleBounceSound = nullptr;
    }
    if (scoreSound)
    {
        MIX_DestroyAudio(scoreSound);
        scoreSound = nullptr;
    }
    if (ball.sprite)
    {
        SDL_DestroyTexture(ball.sprite);
        ball.sprite = nullptr;
    }
    if (paddleSprite)
    {
        SDL_DestroyTexture(paddleSprite);
        paddleSprite = nullptr;
    }
    if (doc)
    {
        doc->Close();
        doc = nullptr;
    }
}

void GameScene::onSecondCounterTimer()
{
    gameTime++;
    multiplier = SDL_log10(gameTime) + 1;
    soloScore += 10 * multiplier;
    UpdateScore(-1);
}

/// This keeps track of the time
/// This static calls the other one
static Uint32 onSecondCounterTimerCallback(void *userdata, SDL_TimerID, Uint32 interval)
{
    GameScene *instance = static_cast<GameScene *>(userdata);
    instance->onSecondCounterTimer();
    return interval;
}

static Size2D GetCurrentRenderSize(const AppContext *app)
{
    int w, h;
    SDL_GetCurrentRenderOutputSize(app->renderer, &w, &h);
    return Size2D{static_cast<float>(w), static_cast<float>(h)};
}

void GameScene::Ready()
{
    // Fonts should be loaded before any documents are loaded.
    if (Rml::LoadFontFace("assets/PixelOperator8.ttf"))
    {
        SDL_LogDebug(SDL_LOG_PRIORITY_DEBUG, "Loaded font");
    }

    // Load the document only once: Close() just moves it to the context's
    // "unloaded_documents" until Rml::Shutdown(), so reloading it on every
    // OnEnter would leak a full element tree per visit.
    if (!doc)
    {
        doc = app->context->LoadDocument("assets/ui/game_screen.rml");
        if (!doc)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't read RmlUi document");
        }
    }

    lastKnownRenderSize = GetCurrentRenderSize(app);

    ball.radius = Radius{std::min(lastKnownRenderSize.width, lastKnownRenderSize.height) / 72};
    ball.rec.w = ball.radius.value * 2;
    ball.rec.h = ball.radius.value * 2;
    paddles[0].rec = {ball.radius.value, lastKnownRenderSize.height * 0.5f, ball.radius.value, ball.radius.value * 8};
    if (gameMode != game::mode::SOLO)
    {
        paddles[1].rec = {lastKnownRenderSize.width - 2 * ball.radius.value, lastKnownRenderSize.height * 0.5f, ball.radius.value, ball.radius.value * 8};
    }

    initialSpeed = lastKnownRenderSize.width / 3;
}

void GameScene::OnEnter()
{
    // SDL_Delay(5000); // Give it a second before starting.
    // On solo mode, setup a second counter to keep the score
    ResetBall();
    scores[0] = 0;
    scores[1] = 0;
    winning_points = 5;

    paddleTouchActive[0] = false;
    paddleTouchActive[1] = false;
    mouseActive[0] = false;
    mouseActive[1] = false;
    paddles[0].direction = 0;
    paddles[1].direction = 0;

    if (gameMode == game::mode::SOLO)
    {
        secondCounterTimer = SDL_AddTimer(1000, onSecondCounterTimerCallback, this);
    }

    UpdateScore(-1);
    if (doc)
    {
        doc->Show();
    }
}

void GameScene::OnExit()
{
    if (gameMode == game::mode::SOLO && secondCounterTimer)
    {
        SDL_RemoveTimer(secondCounterTimer);
        secondCounterTimer = 0;
    }
    if (doc)
    {
        doc->Hide();
    }
}

SDL_AppResult GameScene::HandleEvent(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_FINGER_DOWN:
        ProcessTouch(event->tfinger.x, event->tfinger.y, event->tfinger.fingerID, true);
        break;
    case SDL_EVENT_FINGER_MOTION:
        ProcessTouch(event->tfinger.x, event->tfinger.y, event->tfinger.fingerID, false);
        break;
    case SDL_EVENT_FINGER_UP:
        ReleaseTouch(event->tfinger.fingerID);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT)
        {
            ProcessMouse(event->button.x, event->button.y, true);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        ProcessMouse(event->motion.x, event->motion.y, false);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT)
        {
            ReleaseMouse();
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.scancode)
        {
        case SDL_SCANCODE_ESCAPE:
        case SDL_SCANCODE_AC_BACK:
            core::scene::events::EmitSceneFinishedEvent(); // end the scene
            break;
        case SDL_SCANCODE_W:
            paddles[0].direction = -1;
            break;
        case SDL_SCANCODE_S:
            paddles[0].direction = 1;
            break;
        case SDL_SCANCODE_UP:
        {
            const int playerIndex = gameMode == game::mode::TWO_PLAYERS ? 1 : 0;
            paddles[playerIndex].direction = -1;
            break;
        }
        case SDL_SCANCODE_DOWN:
        {
            const int playerIndex = gameMode == game::mode::TWO_PLAYERS ? 1 : 0;
            paddles[playerIndex].direction = 1;
            break;
        }
        default:
            break;
        }
        break;
    case SDL_EVENT_KEY_UP:
        switch (event->key.scancode)
        {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_S:
            paddles[0].direction = 0;
            break;
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_DOWN:
        {
            const int playerIndex = gameMode == game::mode::TWO_PLAYERS ? 1 : 0;
            paddles[playerIndex].direction = 0;
            break;
        }
        default:
            break;
        }
        break;
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_RESIZED:
        adjustToScreen();
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

void GameScene::Update(float deltatime)
{
    if (timeAfterGameEnded >= 0.0)
    { // If our counter has started
        timeAfterGameEnded += deltatime;
        if (timeAfterGameEnded >= 1.5)
        {                                                  // Wait 1.5 seconds
            core::scene::events::EmitSceneFinishedEvent(); // end the scene
        }
    }
    CheckCollisions();
    // BallMovement
    ball.rec.x += ball.velocity.x * deltatime * ball.speed.value;
    ball.rec.y += ball.velocity.y * deltatime * ball.speed.value;

    // PaddleMovement
    paddles[0].rec.y += paddles[0].direction * paddles[0].speed.value * deltatime;
    // Single Player NPC movement
    if (gameMode == game::mode::SINGLE_PLAYER)
    {
        // Only move if the ball is closer to the 2nd player
        if (ball.rec.x >= lastKnownRenderSize.width * 0.5)
        {
            float target_y = ball.rec.y - paddles[1].rec.h * 0.5;
            float distance = target_y - paddles[1].rec.y;
            paddles[1].rec.y += SDL_clamp(distance, -paddles[1].speed.value * 1.5 * deltatime, paddles[1].speed.value * 1.5 * deltatime);
        }
    }
    else
    { // Second Player movement
        paddles[1].rec.y += paddles[1].direction * paddles[1].speed.value * deltatime;
    }

    paddles[0].rec.y = SDL_clamp(paddles[0].rec.y, 0.0f, lastKnownRenderSize.height - paddles[0].rec.h);
    paddles[1].rec.y = SDL_clamp(paddles[1].rec.y, 0.0f, lastKnownRenderSize.height - paddles[1].rec.h);
}

void GameScene::Render()
{
    SDL_SetRenderDrawColor(app->renderer, 0xC, 0xC, 0xC, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(app->renderer);

    SDL_RenderTexture(app->renderer, paddleSprite, nullptr, &paddles[0].rec);

    if (gameMode != game::mode::SOLO)
    {
        SDL_RenderTexture(app->renderer, paddleSprite, nullptr, &paddles[1].rec);
    }
    SDL_RenderTexture(app->renderer, ball.sprite, nullptr, &ball.rec);

    if (app->context)
    {
        app->context->Update();
        // app->render_interface->BeginFrame();
        app->context->Render();
        // app->render_interface->EndFrame();
    }
    SDL_RenderPresent(app->renderer);
}

void GameScene::ResetBall()
{
    ball.speed.value = initialSpeed;
    paddles[0].speed.value = initialSpeed;
    paddles[1].speed.value = initialSpeed;
    multiplier = 1;
    ball.rec.x = lastKnownRenderSize.width / 2;
    ball.rec.y = lastKnownRenderSize.height / 2;
    constexpr float PI = SDL_PI_F;
    float angle = 2 * PI * SDL_randf();
    while ((angle >= PI / 3 and angle <= 2 * PI / 3) or (angle >= 4 * PI / 3 and angle <= 5 * PI / 3))
    {
        angle = 2 * PI * SDL_randf();
    }
    ball.velocity.x = SDL_cosf(angle);
    ball.velocity.y = SDL_sinf(angle);
}

static int paddleCooldownFrames = 0;

void GameScene::CheckCollisions()
{
    if (paddleCooldownFrames > 0)
        paddleCooldownFrames--;
    float screenHalf = lastKnownRenderSize.width / 2;
    const bool isInCollitionBounds = ball.rec.x < lastKnownRenderSize.width * 0.15 or ball.rec.x > lastKnownRenderSize.width * 0.85;
    if (isInCollitionBounds && paddleCooldownFrames == 0)
    {
        // Paddle collition
        const int playerIndex = ball.rec.x < screenHalf ? 0 : 1;
        Paddle paddle = paddles[playerIndex];
        if (SDL_HasRectIntersectionFloat(&ball.rec, &paddles[playerIndex].rec))
        {
            if (paddleBounceTrack)
            {
                MIX_PlayTrack(paddleBounceTrack, 0);
            }
            // Change bounce depending on impact zone
            const float paddleCenterY = paddle.rec.y + paddle.rec.h / 2;
            float offset = (ball.rec.y - paddleCenterY) / (paddle.rec.h / 2); // Range: -1 to 1
            offset = SDL_clamp(offset, -1.0f, 1.0f);
            // Bounce angle (-45° to 45°)
            const float angle = offset * SDL_PI_F / 4;
            const float direction = ball.velocity.x > 0 ? -1.0 : 1.0;
            ball.velocity.x = SDL_cosf(angle) * direction;
            ball.velocity.y = SDL_sinf(angle);
            // Speed up
            ball.speed.value += ball.radius.value;
            paddle.speed.value += ball.radius.value / 5;
            soloScore += multiplier * 50;
            // paddleSound
            paddleCooldownFrames = 30;
        }
    }

    // # Collision
    // ball_hitbox = Rect2(Ball.position - Vector2(radius, radius), Vector2(radius * 2, radius * 2))
    // for i in PaddleList.size():
    // 	var paddle = PaddleList[i]
    // 	if ball_hitbox.intersects(paddle):
    // 		# Change bounce depending on impact zone
    // 		var paddle_center_y = paddle.position.y + paddle.size.y / 2
    // 		var offset = (Ball.position.y - paddle_center_y) / (paddle.size.y / 2)  # Range: -1 to 1
    // 		offset = clamp(offset, -1.0, 1.0)
    // 		# Bounce angle (-45° a 45°)
    // 		var angle = offset * deg_to_rad(45)
    // 		var direction = sign(ball_movement.x) * -1  # Change direction
    // 		ball_movement = Vector2(cos(angle) * direction, sin(angle)).normalized()
    // 		# Speed up
    // 		ball_speed += 20
    // 		paddle_speed += 5
    // 		score += multiplier * 50
    // 		# Pong
    // 		audio_player.stream = pong
    // 		audio_player.play()

    // World Boundaries
    if (ball.rec.x + ball.radius.value >= lastKnownRenderSize.width)
    {
        if (gameMode == game::mode::SOLO)
        {
            ball.velocity.x *= -1;
            if (wallBounceTrack)
            {
                MIX_PlayTrack(wallBounceTrack, 0);
            }
        }
        else
        {
            UpdateScore(0);
        }
    }
    else if (ball.rec.x <= ball.radius.value)
    {
        UpdateScore(1);
    }

    // # World Boundaries
    // if (Ball.position.x + radius >= viewport_bounds.x ):
    // 	if (game_mode == game::mode.SOLO):
    // 		ball_movement.x *= -1
    // 		audio_player.stream = ping
    // 		audio_player.play()
    // 	else:
    // 		score_goal(0)
    // elif (Ball.position.x <= radius):
    // 	score_goal(1)

    bool outOfBoundsY = ball.rec.y + ball.radius.value >= lastKnownRenderSize.height or ball.rec.y <= ball.radius.value;
    if (outOfBoundsY)
    {
        // wallSound
        ball.velocity.y *= -1;
        ball.speed.value += ball.radius.value / 5;
        if (wallBounceTrack)
        {
            MIX_PlayTrack(wallBounceTrack, 0);
        }
    }
    // var out_bounds_y : bool = Ball.position.y + radius >= viewport_bounds.y or Ball.position.y + radius <= radius
    // if(out_bounds_y):
    // 	audio_player.stream = ping
    // 	audio_player.play()
    // 	ball_movement.y *= -1
    // 	ball_speed += 5
    // Ball.position += ball_movement * delta * ball_speed
}

SDL_Texture *GameScene::LoadImageTexture(const std::string &path)
{
    SDL_Surface *surface = IMG_Load(path.c_str());
    if (!surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load image: %s", SDL_GetError());
        return nullptr;
    }

    SDL_Texture *imageTex = SDL_CreateTextureFromSurface(app->renderer, surface);
    SDL_DestroySurface(surface);

    if (!imageTex)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create image texture: %s", SDL_GetError());
        return nullptr;
    }

    return imageTex;
}

void GameScene::adjustToScreen()
{
    Size2D newRenderSize = GetCurrentRenderSize(app);
    float xDiff = newRenderSize.width / lastKnownRenderSize.width;
    float yDiff = newRenderSize.height / lastKnownRenderSize.height;
    initialSpeed *= xDiff;
    ball.speed.value *= xDiff;
    paddles[0].speed.value *= yDiff;
    paddles[1].speed.value *= yDiff;
    ball.radius = Radius{std::min(lastKnownRenderSize.width, lastKnownRenderSize.height) / 72};
    ball.rec.w = ball.radius.value * 2;
    ball.rec.h = ball.radius.value * 2;
    paddles[0].rec.w = ball.radius.value;
    paddles[0].rec.h = ball.radius.value * 8;
    paddles[0].rec.y *= yDiff;
    if (gameMode != game::mode::SOLO)
    {
        paddles[1].rec.w = ball.radius.value;
        paddles[1].rec.h = ball.radius.value * 8;
        paddles[1].rec.x *= xDiff;
        paddles[1].rec.y *= yDiff;
    }
    ball.rec.x *= xDiff;
    ball.rec.y *= yDiff;
    lastKnownRenderSize = newRenderSize;
}

void GameScene::UpdateScoreDisplay()
{
    if (!doc)
        return;
    Rml::Element *score_label = doc->GetElementById("score");
    if (!score_label or timeAfterGameEnded >= 0.0)
        return;

    std::string scoreText;
    if (gameMode == game::mode::SOLO)
    {
        scoreText = std::format("Ball: {} | Score: {:06d}", winning_points - scores[1], soloScore);
    }
    else
    {
        scoreText = std::format("{:02d} | {:02d}", scores[0], scores[1]);
    }

    score_label->SetInnerRML(scoreText);
}

void GameScene::CheckGameOver()
{
    if (scores[0] < winning_points && scores[1] < winning_points)
    {
        ResetBall(); // No ha terminado el juego
        return;
    }

    // Fin del juego
    ball.speed.value = 0;
    ball.rec.x = lastKnownRenderSize.width / 2;
    ball.rec.y = lastKnownRenderSize.height / 2;

    if (!doc)
        return;
    Rml::Element *score_label = doc->GetElementById("score");
    if (!score_label)
        return;

    std::string scoreText;
    if (gameMode == game::mode::SOLO)
    {
        scoreText = std::format("Final Score: {}", soloScore);
    }
    else
    {
        const int winner = scores[0] > scores[1] ? 1 : 2;
        scoreText = std::format("P{} WINS", winner);
    }

    score_label->SetInnerRML(scoreText);
    timeAfterGameEnded = 0.0f;
}

void GameScene::UpdateScore(int scorerIndex)
{
    // Si no es inicialización (-1), incrementa el marcador del jugador
    if (scorerIndex >= 0)
    {
        scores[scorerIndex]++;
        if (scoreTrack)
        {
            MIX_PlayTrack(scoreTrack, 0);
        }
        UpdateScoreDisplay();
        CheckGameOver();
    }
    else
    {
        UpdateScoreDisplay();
    }
}

void GameScene::ProcessTouch(float normX, float normY, SDL_FingerID fingerId, bool isDown)
{
    float pixelX = normX * lastKnownRenderSize.width;
    float pixelY = normY * lastKnownRenderSize.height;

    int playerIdx = 0;
    if (isDown)
    {
        if (gameMode == game::mode::TWO_PLAYERS)
        {
            playerIdx = (pixelX < lastKnownRenderSize.width * 0.5f) ? 0 : 1;
        }
        paddleFinger[playerIdx] = fingerId;
        paddleTouchActive[playerIdx] = true;
    }
    else
    {
        // On motion: bind to whichever paddle is already tracking this finger
        if (gameMode == game::mode::TWO_PLAYERS)
        {
            if (paddleTouchActive[0] && paddleFinger[0] == fingerId)
            {
                playerIdx = 0;
            }
            else if (paddleTouchActive[1] && paddleFinger[1] == fingerId)
            {
                playerIdx = 1;
            }
            else
            {
                playerIdx = (pixelX < lastKnownRenderSize.width * 0.5f) ? 0 : 1;
            }
        }
        else
        {
            playerIdx = 0;
        }
    }

    if (paddleTouchActive[playerIdx] && paddleFinger[playerIdx] == fingerId)
    {
        paddles[playerIdx].rec.y = pixelY - paddles[playerIdx].rec.h * 0.5f;
        paddles[playerIdx].rec.y = SDL_clamp(paddles[playerIdx].rec.y, 0.0f, lastKnownRenderSize.height - paddles[playerIdx].rec.h);
    }
}

void GameScene::ReleaseTouch(SDL_FingerID fingerId)
{
    for (int i = 0; i < 2; ++i)
    {
        if (paddleTouchActive[i] && paddleFinger[i] == fingerId)
        {
            paddleTouchActive[i] = false;
        }
    }
}

void GameScene::ProcessMouse(float pixelX, float pixelY, bool isDown)
{
    int playerIdx = 0;
    if (gameMode == game::mode::TWO_PLAYERS)
    {
        playerIdx = (pixelX < lastKnownRenderSize.width * 0.5f) ? 0 : 1;
    }

    if (isDown)
    {
        mouseActive[playerIdx] = true;
    }

    if (mouseActive[playerIdx])
    {
        paddles[playerIdx].rec.y = pixelY - paddles[playerIdx].rec.h * 0.5f;
        paddles[playerIdx].rec.y = SDL_clamp(paddles[playerIdx].rec.y, 0.0f, lastKnownRenderSize.height - paddles[playerIdx].rec.h);
    }
}

void GameScene::ReleaseMouse()
{
    mouseActive[0] = false;
    mouseActive[1] = false;
}
