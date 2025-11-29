#ifndef CORE_SCENE_STATE_MACHINE_H
#define CORE_SCENE_STATE_MACHINE_H

#include <string>
#include <unordered_map>
#include <SDL3/SDL.h>
#include "Manager.h"
#include "Events.h"

namespace core
{
    namespace scene
    {

        /**
         * @brief Simple finite state machine to control scene transitions.
         * It listens for scene events and triggers scene changes via Manager.
         */
        class StateMachine
        {
        private:
            Manager &sceneManager;
            std::unordered_map<std::string, std::string> sceneTransitions; // fromScene -> toScene
            std::string currentSceneName;

        public:
            explicit StateMachine(Manager &manager);

            /// Non-copyable
            StateMachine(const StateMachine &) = delete;
            StateMachine &operator=(const StateMachine &) = delete;

            /**
             * @brief Define a transition from one scene to another.
             * @param from Name of the scene that triggers the transition.
             * @param to Name of the next scene.
             */
            void AddTransition(const std::string &from, const std::string &to);

            /**
             * @brief Initialize FSM and activate the starting scene.
             * @param startScene Name of the initial scene to enter.
             * @return true if successful.
             */
            bool Init(const std::string &startScene);

            /**
             * @brief Handle an SDL event. If it's a scene finished event, trigger transition.
             * @param event The SDL event.
             */
            void HandleEvent(const SDL_Event &event);

            /**
             * @brief Update current scene logic.
             * @param deltaTime Time elapsed since last update.
             */
            void Update(float deltaTime);

            /**
             * @brief Render the current scene.
             */
            void Render();

            /**
             * @brief Get the name of the current active scene.
             * @return Current scene name.
             */
            std::string GetCurrentSceneName() const;
        };

    } // namespace scene
} // namespace core

#endif // CORE_SCENE_STATE_MACHINE_H
