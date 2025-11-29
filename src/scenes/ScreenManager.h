#include "core/scene/Manager.h"
#include "core/scene/Events.h"
#include "core/AppContext.h"
#include "scenes/SplashScene.h"
#include "scenes/MainMenuScene.h"
#include "scenes/GameScene.h"

/// @brief This function initialices the Global SceneManager.
/// It should be called once (and only once during runtime) in the SDL_AppInit function.
/// @param screenManager
/// @param app
/// @return
bool InitScreenManager(core::scene::Manager *screenManager, AppContext *app)
{
    // Register ALL of the subscene events, even if they should not be called yet.
    core::scene::events::RegisterCommonSceneEvents();
    game::menu::RegisterMainMenuEvents();

    screenManager->RegisterScene(std::make_unique<SplashScene>(app));
    screenManager->RegisterScene(std::make_unique<MainMenuScene>(app));

    if (!screenManager->InitScenes())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialize initial scenes");
        return false;
    }
    if (!screenManager->ChangeScene("Splash"))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't enter first scene");
        return false;
    }
    return true;
};

SDL_AppResult HandleScreenEvents(SDL_Event *event, core::scene::Manager *sceneManager, AppContext *app)
{
    if (event->type == core::scene::events::SCENE_FINISHED)
    {
        const std::string currentScene = sceneManager->GetCurrentSceneName();
        if (currentScene == "Splash")
        {
            sceneManager->RemoveScene("Splash");
            sceneManager->ChangeScene("MainMenu");
            return SDL_APP_CONTINUE;
        }
        else if (currentScene == "Game")
        {
            sceneManager->RemoveScene("Game");
            sceneManager->ChangeScene("MainMenu");
            return SDL_APP_CONTINUE;
        }
        else if (currentScene == "MainMenu")
        {
            SDL_Log("Ending from Main Menu");
            sceneManager->CleanUp();
            return SDL_APP_SUCCESS;
        }
        else
        {
            std::string err = ("Scene %s ended unexpectedly", sceneManager->GetCurrentSceneName());
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, err.c_str());
            sceneManager->CleanUp();
            return SDL_APP_SUCCESS;
        }
    }
    else if (event->type == game::menu::START_GAME)
    {
        SDL_LogDebug(SDL_LOG_PRIORITY_TRACE,"Event Handled: START_GAME");
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,"Starting with %d mode", (game::mode::Mode)(event->user.code));
        auto menu = std::make_unique<GameScene>(
            app,
            static_cast<game::mode::Mode>(event->user.code));

        const bool ok = sceneManager->RegisterAndInitScene(std::make_unique<GameScene>(
            app,
            static_cast<game::mode::Mode>(event->user.code)));
        if (ok)
        {
            if (sceneManager->ChangeScene("Game"))
            {
                return SDL_APP_CONTINUE;
            }
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ScreenManager: Couldn't init Game screen");
        }
        return SDL_APP_FAILURE;
    }
    else
    {
        return sceneManager->HandleEvent(event);
    }
};
