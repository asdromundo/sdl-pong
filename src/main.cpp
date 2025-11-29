#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
// #include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <filesystem>

#include "scenes/ScreenManager.h"

// RmlUi
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#ifndef NDEBUG
#include <RmlUi/Debugger.h>
#endif
#include "rmlui/RmlUi_Platform_SDL.h"
#include "rmlui/RmlUi_Renderer_SDL.h"

constexpr uint32_t windowStartWidth = 1280;
constexpr uint32_t windowStartHeight = 720;

Uint64 lastTick = 0;
Uint64 currentTick = 0;
float delta_time = 0;

core::scene::Manager *screenManager{nullptr};

SDL_AppResult SDL_Fail()
{
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    // init the library, here we make a window so we only need the Video capabilities.
    if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        return SDL_Fail();
    }

    // init TTF
    // if (not TTF_Init())
    // {
    //     return SDL_Fail();
    // }

    // create a window

    SDL_Window *window = SDL_CreateWindow("Pong", windowStartWidth, windowStartHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (not window)
    {
        return SDL_Fail();
    }

    // Carga la imagen con SDL_image (si usas PNG u otros)
    SDL_Surface *icon = IMG_Load("resources/logo.svg");
    if (icon)
    {
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    }
    else
    {
        SDL_Log("Failed to load icon: %s", SDL_GetError());
    }

    // SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    // create a renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (not renderer)
    {
        return SDL_Fail();
    }

    // init SDL Mixer
    auto audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (not audioDevice)
    {
        return SDL_Fail();
    }
    if (not Mix_OpenAudio(audioDevice, NULL))
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
    };

    SDL_SetRenderVSync(renderer, -1); // enable vysnc

    SDL_Log("Application started successfully!");

    // RmlUi
    // Submit click events when focusing the window.
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    // Instantiate the interfaces to RmlUi.
    auto app = (AppContext *)*appstate;
    app->render_interface = new RenderInterface_SDL(renderer);
    app->system_interface = new SystemInterface_SDL();
    app->system_interface->SetWindow(window);

    // Begin by installing the custom interfaces.
    Rml::SetRenderInterface(app->render_interface);
    Rml::SetSystemInterface(app->system_interface);

    if (app->system_interface->LogMessage(Rml::Log::LT_INFO, Rml::CreateString("Using SDL renderer: %s", SDL_GetRendererName(app->renderer))))
    {
        SDL_Log(SDL_GetRendererName(app->renderer));
    }
    // Now we can initialize RmlUi.
    Rml::Initialise();
    Rml::Log::Message(Rml::Log::LT_WARNING, "Test warning.");

    // Create a context next.
    Rml::Context *context = Rml::CreateContext("main", Rml::Vector2i(windowStartWidth, windowStartHeight), app->render_interface);
    if (!context)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't create RmlUi context");
        Rml::Shutdown();
        return SDL_Fail();
    }

// If you want to use the debugger, initialize it now.
#ifndef NDEBUG
    Rml::Debugger::Initialise(context);
#endif

    // Fonts should be loaded before any documents are loaded.
    // if (Rml::LoadFontFace("resources/monogram.ttf"))
    // {
    //     SDL_Log("Loaded font");
    // }

    // Now we are ready to load our document.
    // Rml::ElementDocument *document = context->LoadDocument("resources/ui/test.rml");
    // if (!document)
    // {
    //     SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't read RmlUi document");
    //     Rml::Shutdown();
    //     return SDL_Fail();
    // }
    // document->Show();
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
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_RESIZED:
        int w, h;
        SDL_GetCurrentRenderOutputSize(app->renderer, &w, &h);
        app->context->SetDimensions(Rml::Vector2i(w, h));
        break;
    case SDL_EVENT_QUIT:
        app->app_quit = SDL_APP_SUCCESS;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        app->context->ProcessMouseMove(event->motion.x, event->motion.y, 0);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        int button = event->button.button;

        // SDL: 1=izq, 2=medio, 3=der. RmlUi usa 0=izq, 1=medio, 2=der
        int rml_button = button - 1;

        app->context->ProcessMouseButtonDown(rml_button, 0);
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        int button = event->button.button;
        int rml_button = button - 1;

        app->context->ProcessMouseButtonUp(rml_button, 0);
        break;
    }
    case SDL_EVENT_KEY_DOWN:
    {
        const SDL_Keycode keycode = event->key.key;

        // Convierte la tecla SDL a RmlUi
        Rml::Input::KeyIdentifier rml_key = RmlSDL::ConvertKey(keycode);
        app->context->ProcessKeyDown(rml_key, 0);

        // Simula un Enter si hace falta para que dispare eventos "click"
        if (keycode == SDLK_RETURN || keycode == SDLK_KP_ENTER)
        {
            app->context->ProcessTextInput("\n");
        }

        switch (event->key.scancode)
        {
#ifndef NDEBUG
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

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    auto *app = (AppContext *)appstate;
    if (app)
    {
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);

        Mix_FadeOutMusic(1000); // prevent the music from abruptly ending.
        Mix_CloseAudio();
        SDL_CloseAudioDevice(app->audioDevice);
        SDL_Log("Closing app");
        Rml::Shutdown();
        delete app->render_interface;
        delete app->system_interface;

        delete app;
    }
    // TTF_Quit();
    Mix_Quit();
    SDL_Log("Application quit successfully!\n");
    SDL_Quit();
}