#ifndef SCENES_MAIN_MENU_SCENE_H
#define SCENES_MAIN_MENU_SCENE_H

#include "core/scene/Scene.h"
#include "game/Mode.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/EventListener.h> // <-- Necesario si usas custom EventListener

namespace game::menu
{
    inline Uint32 START_GAME = 0;

    inline bool RegisterMainMenuEvents()
    {
        int numberOfEvents = 1;
        Uint32 baseEvent = SDL_RegisterEvents(numberOfEvents);
        if (baseEvent == static_cast<Uint32>(-1))
        {
            return false;
        }

        START_GAME = baseEvent++;
        return true;
    }

    inline bool EmitStartGameEvent(game::mode::Mode mode)
    {
        SDL_Event event{};
        SDL_zero(event);
        event.type = START_GAME;
        event.user.code = static_cast<Sint32>(mode);
        return SDL_PushEvent(&event) == 1;
    }

} // namespace game::menu

class MainMenuScene : public core::scene::Scene
{
public:
    explicit MainMenuScene(AppContext *context);
    ~MainMenuScene() override;

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
    Mix_Chunk *moveSound{nullptr};
    Mix_Chunk *enterSound{nullptr};

private:
    SDL_Texture *messageTex{nullptr};
    SDL_Texture *imageTex{nullptr};
    Mix_Music *music{nullptr};
    SDL_FRect messageDest{};
    // RmlUi
    Rml::ElementDocument *doc{nullptr};

    bool LoadImageTexture(const std::string &path);
    bool LoadMusic(const std::string &path);
};

#endif // SCENES_MAIN_MENU_SCENE_H
