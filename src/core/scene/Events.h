#ifndef CORE_SCENE_EVENTS_H
#define CORE_SCENE_EVENTS_H

#include <SDL3/SDL.h>

namespace core::scene::events {

    /**
     * @brief Event type emitted by scenes to notify the application that the scene has completed.
     * The scene state machine or app logic decides what to do next.
     */
    inline Uint32 SCENE_FINISHED = 0;

    /**
     * @brief Registers all custom scene-related SDL events.
     * Must be called once during application startup (e.g., in SDL_AppInit).
     * 
     * @return true if registration was successful; false otherwise.
     */
    inline bool RegisterCommonSceneEvents() {
        // SDL_RegisterEvents returns the first ID of the registered range, or (Uint32)-1 on failure
        Uint32 baseEvent = SDL_RegisterEvents(1);
        if (baseEvent == static_cast<Uint32>(-1)) {
            return false; // Registration failed
        }

        SCENE_FINISHED = baseEvent;
        return true;
    }

    /**
     * @brief Emits a SCENE_FINISHED event to signal that the current scene has completed.
     * Should be called by a scene when it's ready to transition out.
     * 
     * @return true if the event was successfully pushed to the SDL event queue.
     */
    inline bool EmitSceneFinishedEvent() {
        SDL_Event event{};
        event.type = SCENE_FINISHED;
        return SDL_PushEvent(&event) == 1;
    }

} // namespace core::scene::events

#endif // CORE_SCENE_EVENTS_H
