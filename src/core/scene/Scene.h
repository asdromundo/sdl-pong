#ifndef CORE_SCENE_H
#define CORE_SCENE_H

#include <string>
#include "core/AppContext.h"

namespace core
{
    namespace scene
    {

        /**
         * @brief Abstract base class representing a game or application scene.
         * Defines the lifecycle and key functions that derived scenes must implement.
         */
        class Scene
        {
        protected:
            const AppContext *app{nullptr}; ///< Application context (read-only).
            std::string sceneName;          ///< Identifier name of the scene.

        public:
            /**
             * @brief Constructor that injects the application context and assigns a name to the scene.
             * @param name The name of the sc ene.
             * @param context Pointer to the global application context.
             */
            Scene(const std::string &name, AppContext *context)
                : app(context), sceneName(name) {}

            /// Delete default constructor to enforce context injection
            Scene() = delete;

            /// Virtual default destructor
            virtual ~Scene() = default;

            /// Deleted copy constructor and assignment to prevent unintentional duplication
            Scene(const Scene &) = delete;
            Scene &operator=(const Scene &) = delete;

            /**
             * @brief Returns the scene's name.
             * @return The scene name.
             */
            std::string GetName() const { return sceneName; }

            // Scene lifecycle methods

            /**
             * @brief Initialize persistent or context-independent resources.
             * @return true if initialization succeeded; false otherwise.
             */
            virtual bool Init() = 0;

            /**
             * @brief Called when the scene is fully initialized.
             * Ideal for logic that depends on all resources being ready.
             */
            virtual void Ready() = 0;

            /**
             * @brief Called when the scene becomes active (entered).
             * You can reset dynamic state here.
             */
            virtual void OnEnter() = 0;

            /**
             * @brief Called before the scene is deactivated (exited).
             * Ideal for saving state or releasing temporary resources.
             */
            virtual void OnExit() = 0;

            /**
             * @brief Releases all final resources when the scene is no longer needed.
             */
            virtual void CleanUp() = 0;

            // Main loop functions

            /**
             * @brief Handles input events (keyboard, mouse, etc.).
             * @param event The SDL event received.
             */
            virtual SDL_AppResult HandleEvent(SDL_Event *event) = 0;

            /**
             * @brief Updates the scene logic based on delta time.
             * @param deltaTime Time elapsed since the last update.
             */
            virtual void Update(float deltaTime) = 0;

            /**
             * @brief Renders the scene.
             */
            virtual void Render() = 0;
        };

    } // namespace scene
} // namespace core

#endif // CORE_SCENE_H
