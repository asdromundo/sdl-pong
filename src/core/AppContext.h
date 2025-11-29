#ifndef CORE_APP_CONTEXT_H
#define CORE_APP_CONTEXT_H

#include <SDL3_mixer/SDL_mixer.h>
#include "rmlui/RmlUi_Platform_SDL.h"
#include "rmlui/RmlUi_Renderer_SDL.h"

struct AppContext {
    SDL_Window* window{nullptr};
    SDL_Renderer* renderer{nullptr};
    SDL_AudioDeviceID audioDevice{};
    SDL_AppResult app_quit{SDL_APP_CONTINUE};
    RenderInterface_SDL* render_interface{nullptr};
    SystemInterface_SDL* system_interface{nullptr};
    Rml::Context *context;
    // Otros recursos globales que desees...
};

#endif // CORE_APP_CONTEXT_H