#include "core/scene/Manager.h"

namespace core::scene
{

    void Manager::RegisterScene(std::unique_ptr<Scene> scene)
    {
        const std::string &name = scene->GetName();
        scenes[name] = std::move(scene);
    }

    bool Manager::RegisterAndInitScene(std::unique_ptr<Scene> scene)
    {
        const std::string &name = scene->GetName();

        if (!scene->Init())
            return false;

        scenes[name] = std::move(scene);
        return true;
    }

    void Manager::RemoveScene(const std::string &name)
    {
        auto it = scenes.find(name);
        if (it != scenes.end())
        {
            if (currentScene == it->second.get())
            {
                currentScene->OnExit();
                currentScene = nullptr;
            }
            it->second->CleanUp();
            scenes.erase(it);
        }
    }

    bool Manager::ChangeScene(const std::string &name)
    {
        auto it = scenes.find(name);
        if (it == scenes.end())
            return false;

        if (currentScene)
        {
            currentScene->OnExit();
        }

        currentScene = it->second.get();
        currentScene->Ready();
        currentScene->OnEnter();
        return true;
    }

    bool Manager::InitScenes()
    {
        for (auto &[name, scene] : scenes)
        {
            if (!scene->Init())
            {
                return false;
            }
        }
        return true;
    }

    std::string Manager::GetCurrentSceneName() const
    {
        return currentScene ? currentScene->GetName() : "";
    }

    SDL_AppResult Manager::HandleEvent(SDL_Event *event)
    {
        if (currentScene)
        {
            return currentScene->HandleEvent(event);
        }
        return SDL_APP_CONTINUE;
    }

    void Manager::Update(float deltaTime)
    {
        if (currentScene)
        {
            currentScene->Update(deltaTime);
        }
    }

    void Manager::Render()
    {
        if (currentScene)
        {
            currentScene->Render();
        }
    }

    void Manager::CleanUp()
    {
        for (auto &[name, scene] : scenes)
        {
            scene->OnExit();
            scene->CleanUp();
        }
        scenes.clear();
        currentScene = nullptr;
    }

} // namespace core::scene