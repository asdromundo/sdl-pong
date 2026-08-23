#include <SDL3_image/SDL_image.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <cmath>

#include "MainMenuScene.h"
#include "core/scene/Events.h"

class RmlUiEventListener : public Rml::EventListener
{
public:
    explicit RmlUiEventListener(MainMenuScene *scene) : owner(scene) {}
    void ProcessEvent(Rml::Event &event) override
    {
        Rml::Element *target = event.GetCurrentElement();
        std::string id = target->GetId().c_str();

        if (event.GetType() == "focus")
        {
            // Reproduce el sonido al enfocar un botón
            MIX_SetTrackAudio(owner->track1, owner->moveSound);
            MIX_PlayTrack(owner->track1, 0);
            return;
        }

        if (event.GetType() == "click")
        {
            MIX_SetTrackAudio(owner->track1, owner->enterSound);
            MIX_PlayTrack(owner->track1, 0);
            if (id == "solo")
            {
                game::menu::EmitStartGameEvent(game::mode::SOLO);
                SDL_LogDebug(SDL_LOG_CATEGORY_INPUT, "Solo button");
            }
            else if (id == "single")
            {
                game::menu::EmitStartGameEvent(game::mode::SINGLE_PLAYER);
                SDL_LogDebug(SDL_LOG_CATEGORY_INPUT, "Single Player button");
            }
            else if (id == "two")
            {
                game::menu::EmitStartGameEvent(game::mode::TWO_PLAYERS);
                SDL_LogDebug(SDL_LOG_CATEGORY_INPUT, "Two Players button");
            }
        }
    }

private:
    MainMenuScene *owner;
};

MainMenuScene::MainMenuScene(AppContext *context)
    : Scene("MainMenu", context) {}

MainMenuScene::~MainMenuScene()
{
    CleanUp();
}

bool MainMenuScene::Init()
{
    bool ok =
        LoadImageTexture("assets/pong_logo.png");

    track1 = MIX_CreateTrack(app->mixer);
    track2 = MIX_CreateTrack(app->mixer);
    musicTrack = MIX_CreateTrack(app->mixer);
    moveSound = MIX_LoadAudio(app->mixer, "assets/sounds/ping.wav", false);
    enterSound = MIX_LoadAudio(app->mixer, "assets/sounds/pong.wav", false);

    // LoadMusic((basePath / "assets/sounds/the_entertainer.ogg").string());

    return ok;
}

void MainMenuScene::Ready()
{
    // Fonts should be loaded before any documents are loaded.
    if (Rml::LoadFontFace("assets/monogram.ttf"))
    {
        SDL_LogDebug(SDL_LOG_PRIORITY_DEBUG, "Loaded font");
    }
}

void MainMenuScene::OnEnter()
{
    if (music)
    {
        MIX_SetTrackAudio(musicTrack, music);
        MIX_PlayTrack(musicTrack, -1);
    }

    doc = app->context->LoadDocument("assets/ui/main_menu_screen.rml");
    if (!doc)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't read RmlUi document");
    }
    else
    {
        doc->Show();
        // Conectar eventos a botones
        Rml::Element *btn_solo = doc->GetElementById("solo");
        Rml::Element *btn_single = doc->GetElementById("single");
        Rml::Element *btn_two = doc->GetElementById("two");
        RmlUiEventListener *listener = new RmlUiEventListener(this);
        if (btn_solo)
        {
            btn_solo->Focus();
            btn_solo->SetPseudoClass("focus-visible", true);
            btn_solo->AddEventListener("click", listener);
            btn_solo->AddEventListener("focus", listener);
        }
        if (btn_single)
        {
            btn_single->AddEventListener("click", listener);
            btn_single->AddEventListener("focus", listener);
        }
        if (btn_two)
        {
            btn_two->AddEventListener("click", listener);
            btn_two->AddEventListener("focus", listener);
        }
    }

    // End scene after timer
    // SDL_AddTimer(200, SceneStartGameCallback, nullptr);
}

void MainMenuScene::OnExit()
{
    MIX_StopTrack(musicTrack, 10);

    if (doc)
    {
        doc->Close();
    }
}

void MainMenuScene::CleanUp()
{
    if (messageTex)
    {
        SDL_DestroyTexture(messageTex);
        messageTex = nullptr;
    }
    if (imageTex)
    {
        SDL_DestroyTexture(imageTex);
        imageTex = nullptr;
    }
    if (music)
    {
        MIX_DestroyAudio(music);
        music = nullptr;
    }
    if (moveSound)
    {
        MIX_DestroyAudio(moveSound);
        moveSound = nullptr;
    }
    if (enterSound)
    {
        MIX_DestroyAudio(enterSound);
        enterSound = nullptr;
    }
}

SDL_AppResult MainMenuScene::HandleEvent(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.scancode)
        {
        case SDL_SCANCODE_ESCAPE:
            core::scene::events::EmitSceneFinishedEvent(); // end the scene
            break;
        // case SDL_SCANCODE_UP:

        //     app->context->ProcessKeyDown();
        //     break;
        default:
            break;
        }
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_RESTORED:
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

void MainMenuScene::Update(float)
{
    // Add animation or logic if needed
}

void MainMenuScene::Render()
{
    SDL_SetRenderDrawColor(app->renderer, 0x21, 0x21, 0x21, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(app->renderer);

    int targetWidth, targetHeight;

    SDL_GetCurrentRenderOutputSize(app->renderer, &targetWidth, &targetHeight);

    // Relación de aspecto de la imagen (8:3)
    const float aspectRatio = 8.0f / 3.0f;

    float drawWidth, drawHeight;

    // Si la pantalla es más alta que ancha, limitamos el ancho a la mitad del total
    if (targetHeight > targetWidth)
    {
        drawWidth = targetWidth * 0.5f;
        drawHeight = drawWidth / aspectRatio;
    }
    else
    {
        // Si es más ancha que alta, limitamos el alto a la mitad del total
        drawHeight = targetHeight * 0.45f;
        drawWidth = drawHeight * aspectRatio;
    }

    // Calcular coordenadas para centrar
    float dstX = (targetWidth - drawWidth) / 2.0f;
    float dstY = (targetHeight - drawHeight) * 0.125f;

    SDL_FRect dstRect = {
        dstX,
        dstY,
        drawWidth,
        drawHeight};

    SDL_RenderTexture(app->renderer, imageTex, nullptr, &dstRect);

    if (messageTex)
        SDL_RenderTexture(app->renderer, messageTex, nullptr, &messageDest);

    if (app->context)
    {
        app->context->Update();
        // app->render_interface->BeginFrame();
        app->context->Render();
        // app->render_interface->EndFrame();
    }

    SDL_RenderPresent(app->renderer);
}

// Utility loaders

bool MainMenuScene::LoadImageTexture(const std::string &path)
{
    SDL_Surface *surface = IMG_Load(path.c_str());
    if (!surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load image: %s", SDL_GetError());
        return false;
    }

    imageTex = SDL_CreateTextureFromSurface(app->renderer, surface);
    SDL_DestroySurface(surface);

    if (!imageTex)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create image texture: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool MainMenuScene::LoadMusic(const std::string &path)
{
    music = MIX_LoadAudio(app->mixer, path.c_str(), false);
    if (!music)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load music: %s", SDL_GetError());
        return false;
    }
    return true;
}
