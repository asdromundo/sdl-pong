#include <SDL3_image/SDL_image.h>
#include <filesystem>
#include <cmath>

#include "IntroScene.h"

IntroScene::IntroScene(AppContext *context)
    : Scene("Intro", context) {}

IntroScene::~IntroScene()
{
    CleanUp();
}

bool IntroScene::Init()
{
    std::filesystem::path basePath = SDL_GetBasePath();
    if (basePath.empty())
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get base path: %s", SDL_GetError());
        return false;
    }

    musicTrack = MIX_CreateTrack(app->mixer);

    bool ok =
        LoadImageTexture((basePath / "gs_tiger.svg").string()) &&
        LoadMusic((basePath / "the_entertainer.ogg").string());

    return ok;
}

void IntroScene::Ready()
{
}

void IntroScene::OnEnter()
{
    if (music)
    {
        MIX_SetTrackAudio(musicTrack, music);
        MIX_PlayTrack(musicTrack, -1);
    }
    else
    {
        SDL_Log("No music for the %s scene.", GetName().c_str());
    }
}

void IntroScene::OnExit()
{
    MIX_StopTrack(musicTrack, 10);
}

void IntroScene::CleanUp()
{
    if (messageTex)
    {
        SDL_DestroyTexture(messageTex);
        messageTex = nullptr;
    }
    if (imageTex)
    {
        SDL_DestroyTexture(imageTex);
        imageTex = nullptr;
    }
    if (music)
    {
        MIX_DestroyAudio(music);
        music = nullptr;
    }
}

SDL_AppResult IntroScene::HandleEvent(SDL_Event *)
{
    return SDL_APP_CONTINUE;
}

void IntroScene::Update(float)
{
    // Add animation or logic if needed
}

void IntroScene::Render()
{
    float time = SDL_GetTicks() / 1000.0f;
    Uint8 r = Uint8((std::sin(time) + 1.0f) * 0.5f * 255);
    Uint8 g = Uint8((std::sin(time / 2.0f) + 1.0f) * 0.5f * 255);
    Uint8 b = Uint8((std::sin(time * 2.0f) + 1.0f) * 0.5f * 255);

    SDL_SetRenderDrawColor(app->renderer, r, g, b, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(app->renderer);

    if (imageTex)
        SDL_RenderTexture(app->renderer, imageTex, nullptr, nullptr);
    if (messageTex)
        SDL_RenderTexture(app->renderer, messageTex, nullptr, &messageDest);

    SDL_RenderPresent(app->renderer);
}

// Utility loaders
bool IntroScene::LoadImageTexture(const std::string &path)
{
    SDL_Surface *surface = IMG_Load(path.c_str());
    if (!surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load image: %s", SDL_GetError());
        return false;
    }

    imageTex = SDL_CreateTextureFromSurface(app->renderer, surface);
    SDL_DestroySurface(surface);

    if (!imageTex)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create image texture: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool IntroScene::LoadMusic(const std::string &path)
{
    music = MIX_LoadAudio(app->mixer, path.c_str(), false);
    if (!music)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load music: %s", SDL_GetError());
        return false;
    }
    return true;
}
