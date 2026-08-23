#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
// #include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>

#include "scenes/ScreenManager.h"
#include "core/utils/FileSystem.h"

// RmlUi
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#ifndef NDEBUG
#include <RmlUi/Debugger.h>
#endif
#include "rmlui/RmlUi_Platform_SDL.h"
#include "rmlui/RmlUi_Renderer_SDL.h"
#include "rmlui/RmlUi_FileInterface_SDL.h"

constexpr uint32_t windowStartWidth = 1280;
constexpr uint32_t windowStartHeight = 720;

Uint64 lastTick = 0;
Uint64 currentTick = 0;
float delta_time = 0;
float display_scale = 1.0f;

core::scene::Manager *screenManager{nullptr};

SDL_AppResult SDL_Fail()
{
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int, char *[])
{
    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
    if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        return SDL_Fail();
    }

    // utility for asset reading
    FileSystem::Init();
    // create a window

    SDL_Window *window = SDL_CreateWindow("Pong", windowStartWidth, windowStartHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (not window)
    {
        return SDL_Fail();
    }

    SDL_Surface *icon = IMG_Load("assets/pong_logo.png"_asset.c_str());
    if (icon)
    {
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    }
    else
    {
        SDL_Log("Failed to load icon: %s", SDL_GetError());
    }

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");
    // create a renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (not renderer)
    {
        return SDL_Fail();
    }

    // init SDL Mixer
    auto audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (not audioDevice or not MIX_Init())
    {
        return SDL_Fail();
    }
    MIX_Mixer *mixer = MIX_CreateMixerDevice(audioDevice, NULL);
    if (not mixer)
    {
        return SDL_Fail();
    }

    // print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        SDL_Log("Window size: %ix%i", width, height);
        SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
        if (width != bbwidth)
        {
            SDL_Log("This is a highdpi environment.");
        }
    }

    // set up the application data
    *appstate = new AppContext{
        .window = window,
        .renderer = renderer,
        .audioDevice = audioDevice,
        .mixer = mixer,
    };

    SDL_SetRenderVSync(renderer, -1); // enable vysnc

    SDL_Log("Application started successfully!");

    // RmlUi
    // Submit click events when focusing the window.
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    // Instantiate the interfaces to RmlUi.
    auto app = (AppContext *)*appstate;
    app->render_interface = new RenderInterface_SDL(renderer);
    app->system_interface = new SystemInterface_SDL();
    app->file_interface = new FileInterface_SDL();
    app->system_interface->SetWindow(window);

    // Begin by installing the custom interfaces.
    Rml::SetRenderInterface(app->render_interface);
    Rml::SetSystemInterface(app->system_interface);
    Rml::SetFileInterface(app->file_interface);

    if (app->system_interface->LogMessage(Rml::Log::LT_INFO, Rml::CreateString("Using SDL renderer: %s", SDL_GetRendererName(app->renderer))))
    {
        SDL_Log("%s", SDL_GetRendererName(app->renderer));
    }
    // Now we can initialize RmlUi.
    Rml::Initialise();

    display_scale = SDL_GetWindowDisplayScale(window);
    const Rml::Vector2i dim = Rml::Vector2i(windowStartWidth * display_scale, windowStartHeight * display_scale);
    Rml::Context *context = Rml::CreateContext("main", dim, app->render_interface);
    if (!context)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't create RmlUi context");
        Rml::Shutdown();
        return SDL_Fail();
    }
    context->SetDensityIndependentPixelRatio(display_scale);

// If you want to use the debugger, initialize it now.
#ifndef NDEBUG
    Rml::Debugger::Initialise(context);
    SDL_Log("Dim: %d x %d", dim.x, dim.y);
    SDL_Log("Display Scale: %f", context->GetDensityIndependentPixelRatio());
#endif
    app->context = context;

    screenManager = new core::scene::Manager{};
    InitScreenManager(screenManager, (AppContext *)*appstate);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    auto *app = (AppContext *)appstate;

    switch (event->type)
    {
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
    {
        int fb_w, fb_h;
        SDL_GetCurrentRenderOutputSize(app->renderer, &fb_w, &fb_h);
        app->context->SetDimensions(Rml::Vector2i(fb_w, fb_h));

        display_scale = SDL_GetWindowDisplayScale(app->window);
        app->context->SetDensityIndependentPixelRatio(display_scale);
        break;
    }
    case SDL_EVENT_QUIT:
        app->app_quit = SDL_APP_SUCCESS;
        break;
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP:
    {
        // 1. Obtener el tamaño real del canvas
        int fb_w, fb_h;
        SDL_GetCurrentRenderOutputSize(app->renderer, &fb_w, &fb_h);

        // 2. Escalar de [0.0, 1.0] a píxeles físicos
        int x = static_cast<int>(event->tfinger.x * fb_w);
        int y = static_cast<int>(event->tfinger.y * fb_h);

        // 3. Actualizar siempre la posición primero
        app->context->ProcessMouseMove(x, y, 0);

        // 4. Disparar el "clic" izquierdo (botón 0)
        if (event->type == SDL_EVENT_FINGER_DOWN)
        {
            app->context->ProcessMouseButtonDown(0, 0);
        }
        else if (event->type == SDL_EVENT_FINGER_UP)
        {
            app->context->ProcessMouseButtonUp(0, 0);
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
    {
        int x = static_cast<int>(event->motion.x * display_scale);
        int y = static_cast<int>(event->motion.y * display_scale);
        app->context->ProcessMouseMove(x, y, 0);
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        const float display_scale = app->context->GetDensityIndependentPixelRatio();
        int x = static_cast<int>(event->button.x * display_scale);
        int y = static_cast<int>(event->button.y * display_scale);
        app->context->ProcessMouseMove(x, y, 0);

        int rml_button = event->button.button - 1;
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            app->context->ProcessMouseButtonDown(rml_button, 0);
        else
            app->context->ProcessMouseButtonUp(rml_button, 0);
        break;
    }

    case SDL_EVENT_KEY_DOWN:
    {
        const SDL_Keycode keycode = event->key.key;

        // SDL key to RmlUi
        Rml::Input::KeyIdentifier rml_key = RmlSDL::ConvertKey(keycode);
        app->context->ProcessKeyDown(rml_key, 0);

        // If needed simulate Enter
        if (keycode == SDLK_RETURN || keycode == SDLK_KP_ENTER)
        {
            app->context->ProcessTextInput("\n");
        }

        switch (event->key.scancode)
        {
#ifndef NDEBUG
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_F8:
            SDL_LogDebug(SDL_LOG_CATEGORY_RENDER, "Changing visibility of Debugger");
            Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
            break;
#endif
        default:
            break;
        }
    }
    default:
        break;
    }

    if (screenManager)
    {
        return HandleScreenEvents(event, screenManager, app);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    auto *app = (AppContext *)appstate;

    lastTick = currentTick;
    currentTick = SDL_GetTicks();
    delta_time = (currentTick - lastTick) * .001f;

    if (screenManager)
    {
        screenManager->Update(delta_time);
        screenManager->Render();
    }

    return app->app_quit;
}

void SDL_AppQuit(void *appstate, SDL_AppResult)
{
    auto *app = (AppContext *)appstate;
    if (app)
    {
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);

        MIX_StopAllTracks(app->mixer, 1000); // prevent the music from abruptly ending.
        MIX_DestroyMixer(app->mixer);
        SDL_CloseAudioDevice(app->audioDevice);
        SDL_Log("Closing app");
        Rml::Shutdown();
        delete app->render_interface;
        delete app->system_interface;

        delete app;
    }
    MIX_Quit();
    SDL_Log("Application quit successfully!\n");
    SDL_Quit();
}
