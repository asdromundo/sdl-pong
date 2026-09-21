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
            if (owner->track1 && owner->moveSound)
            {
                MIX_SetTrackAudio(owner->track1, owner->moveSound);
                MIX_PlayTrack(owner->track1, 0);
            }
            return;
        }

        if (event.GetType() == "click")
        {
            if (owner->track1 && owner->enterSound)
            {
                MIX_SetTrackAudio(owner->track1, owner->enterSound);
                MIX_PlayTrack(owner->track1, 0);
            }
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
    SDL_SetTextureScaleMode(imageTex, SDL_SCALEMODE_NEAREST);

    if (app->mixer)
    {
        track1 = MIX_CreateTrack(app->mixer);
        track2 = MIX_CreateTrack(app->mixer);
        musicTrack = MIX_CreateTrack(app->mixer);
        moveSound = MIX_LoadAudio(app->mixer, "assets/sounds/ping.wav", false);
        enterSound = MIX_LoadAudio(app->mixer, "assets/sounds/pong.wav", false);
    }

    return ok;
}

void MainMenuScene::Ready()
{
    // Fonts should be loaded before any documents are loaded.
    if (Rml::LoadFontFace("assets/PixelOperator8.ttf"))
    {
        SDL_LogDebug(SDL_LOG_PRIORITY_DEBUG, "Loaded font");
    }

    // Load the document only once: Close() just moves it to the context's
    // "unloaded_documents" until Rml::Shutdown(), so reloading it on every
    // OnEnter would leak a full element tree per visit.
    if (!doc)
    {
        doc = app->context->LoadDocument("assets/ui/main_menu_screen.rml");
        if (!doc)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't read RmlUi document");
            return;
        }

        // Conectar eventos a botones (una sola vez, con el documento)
        menuListener = new RmlUiEventListener(this);
        Rml::Element *btn_solo = doc->GetElementById("solo");
        Rml::Element *btn_single = doc->GetElementById("single");
        Rml::Element *btn_two = doc->GetElementById("two");
        if (btn_solo)
        {
            btn_solo->AddEventListener("click", menuListener);
            btn_solo->AddEventListener("focus", menuListener);
        }
        if (btn_single)
        {
            btn_single->AddEventListener("click", menuListener);
            btn_single->AddEventListener("focus", menuListener);
        }
        if (btn_two)
        {
            btn_two->AddEventListener("click", menuListener);
            btn_two->AddEventListener("focus", menuListener);
        }
    }
}

void MainMenuScene::OnEnter()
{
    if (musicTrack && music)
    {
        MIX_SetTrackAudio(musicTrack, music);
        MIX_PlayTrack(musicTrack, -1);
    }

    if (doc)
    {
        doc->Show();
        // Re-enfocar el botón solo al entrar de nuevo
        if (Rml::Element *btn_solo = doc->GetElementById("solo"))
        {
            btn_solo->Focus();
            btn_solo->SetPseudoClass("focus-visible", true);
        }
    }
}

void MainMenuScene::OnExit()
{
    if (musicTrack)
    {
        MIX_StopTrack(musicTrack, 10);
    }

    // El documento persiste: se cierra una sola vez en CleanUp()
    if (doc)
    {
        doc->Hide();
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
    if (musicTrack)
    {
        MIX_DestroyTrack(musicTrack);
        musicTrack = nullptr;
    }
    if (track1)
    {
        MIX_DestroyTrack(track1);
        track1 = nullptr;
    }
    if (track2)
    {
        MIX_DestroyTrack(track2);
        track2 = nullptr;
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
    if (doc)
    {
        // Desconectar el listener de este documento antes de cerrarlo: el
        // documento sobrevive al Close() en "unloaded_documents" del contexto
        // hasta Rml::Shutdown(), y sus elementos conservan punteros crudos al
        // listener. Si se liberara antes, Rml::Shutdown() llamaría OnDetach()
        // sobre memoria ya liberada.
        if (menuListener)
        {
            const char *buttonIds[] = {"solo", "single", "two"};
            for (const char *id : buttonIds)
            {
                if (Rml::Element *btn = doc->GetElementById(id))
                {
                    btn->RemoveEventListener("click", menuListener);
                    btn->RemoveEventListener("focus", menuListener);
                }
            }
        }
        doc->Close();
        doc = nullptr;
    }
    if (menuListener)
    {
        // El listener ya fue desvinculado de los botones arriba.
        delete menuListener;
        menuListener = nullptr;
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
        case SDL_SCANCODE_AC_BACK:
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

    drawWidth = targetWidth * 0.95f;
    drawHeight = drawWidth / aspectRatio;

    // Centering the image
    float dstX = (targetWidth - drawWidth) * 0.5f;
    float dstY = (targetHeight - drawHeight) * 0.5f;

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
        app->context->Render();
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
    if (!app->mixer)
    {
        return false;
    }
    music = MIX_LoadAudio(app->mixer, path.c_str(), false);
    if (!music)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load music: %s", SDL_GetError());
        return false;
    }
    return true;
}
