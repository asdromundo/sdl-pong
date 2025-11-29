#ifndef SCENES_SPLASH_SCENE_H
#define SCENES_SPLASH_SCENE_H

#include "core/scene/Scene.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>


class SplashScene : public core::scene::Scene {
public:
    explicit SplashScene(AppContext* context);
    ~SplashScene() override;

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

private:
    SDL_Texture* logoTexture{nullptr};

    bool LoadImageTexture(const std::string& path);
    void RenderLogo(SDL_Renderer *renderer);
};

#endif // SCENES_SPLASH_SCENE_H
