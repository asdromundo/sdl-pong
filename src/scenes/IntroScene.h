#ifndef SCENES_INTRO_SCENE_H
#define SCENES_INTRO_SCENE_H

#include "core/scene/Scene.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

class IntroScene : public core::scene::Scene {
public:
    explicit IntroScene(AppContext* context);
    ~IntroScene() override;

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
    SDL_Texture* messageTex{nullptr};
    SDL_Texture* imageTex{nullptr};
    Mix_Music* music{nullptr};
    SDL_FRect messageDest{};

    bool LoadImageTexture(const std::string& path);
    bool LoadMusic(const std::string& path);
};

#endif // SCENES_INTRO_SCENE_H
