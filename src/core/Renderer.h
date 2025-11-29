#include "AppContext.h"
#include <cmath>

class Renderer
{
private:
    const AppContext *app{nullptr};

public:
    ~Renderer() = default;
    Renderer() = delete;
    Renderer(const AppContext *app) : app(app) {};
    void render()
    {
        // draw a color
        auto time = SDL_GetTicks() / 1000.f;
        auto red = (std::sin(time) + 1) / 2.0 * 255;
        auto green = (std::sin(time / 2) + 1) / 2.0 * 255;
        auto blue = (std::sin(time) * 2 + 1) / 2.0 * 255;

        SDL_SetRenderDrawColor(app->renderer, red, green, blue, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(app->renderer);

        SDL_RenderPresent(app->renderer);
    };
};