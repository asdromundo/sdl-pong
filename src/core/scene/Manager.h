#ifndef CORE_SCENE_MANAGER_H
#define CORE_SCENE_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>
#include "Scene.h"

namespace core
{
    namespace scene
    {

        /**
         * @brief Manages the lifecycle and state of registered scenes.
         * Provides basic operations such as changing, removing, and querying scenes by name.
         */
        class Manager
        {
        private:
            std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
            Scene *currentScene{nullptr};

        public:
            Manager() = default;
            ~Manager() = default;

            /// Non-copyable
            Manager(const Manager &) = delete;
            Manager &operator=(const Manager &) = delete;

            /**
             * @brief Registers a scene to be managed.
             * @param scene Unique pointer to the scene (ownership is transferred).
             */
            void RegisterScene(std::unique_ptr<Scene> scene);

            /**
             * @brief Registers a new scene and immediately initializes it.
             * Useful for adding scenes at runtime without reinitializing all.
             * @param scene Unique pointer to the new scene.
             * @return true if initialization succeeded; false otherwise.
             */
            bool RegisterAndInitScene(std::unique_ptr<Scene> scene);

            /**
             * @brief Removes a previously registered scene.
             * If the scene is currently active, it will be exited and deactivated.
             * @param name Name of the scene to remove.
             */
            void RemoveScene(const std::string &name);

            /**
             * @brief Changes the active scene to the one with the given name.
             * Exits the current scene and enters the new one.
             * @param name Name of the scene to activate.
             * @return true if the scene was successfully changed; false otherwise.
             */
            bool ChangeScene(const std::string &name);

            /**
             * @brief Initializes all registered scenes.
             * Should be called once before the main loop.
             * @return true if all scenes initialized successfully.
             */
            bool InitScenes();

            /**
             * @brief Returns the name of the currently active scene.
             * @return Scene name, or empty string if none is active.
             */
            std::string GetCurrentSceneName() const;

            /**
             * @brief Passes the SDL event to the current scene.
             */
            SDL_AppResult HandleEvent(SDL_Event *event);

            /**
             * @brief Updates the current scene.
             * @param deltaTime Time since last update.
             */
            void Update(float deltaTime);

            /**
             * @brief Renders the current scene.
             */
            void Render();

            /**
             * @brief Cleans up all scenes.
             * Called on application shutdown or scene manager destruction.
             */
            void CleanUp();
        };

    } // namespace scene
} // namespace core

#endif // CORE_SCENE_MANAGER_H
