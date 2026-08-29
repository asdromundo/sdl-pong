# SDL3 + RmlUi Multiplatform Game Template

A starting point for C++23 games and UI apps built on:

- **SDL3** — window, rendering, input, audio device
- **RmlUi 6** — HTML/CSS-style UI (`.rml` documents), with a custom SDL3 backend
- **SDL3_mixer** — audio (music + SFX)

The included Pong game (solo, single and two-player modes) is a reference
implementation: it exercises the scene system, RmlUi menus, touch input and
audio on every target.

## Supported platforms

| Platform | Status | How |
| --- | --- | --- |
| Linux, macOS, Windows | tested | CMake, any generator |
| Web (wasm) | tested | Emscripten |
| Android | tested | Gradle project in `android-project/` |
| iOS | scaffolding only, untested | CMake bundle setup present |

## Building

All dependencies (SDL3, SDL3_mixer, SDL3_image, RmlUi) are fetched via CMake
`FetchContent` — no submodules. `find_package` is tried first, so
system-installed copies are used when available. The first configure downloads
and builds everything, so it is slow.

### Desktop (Linux / macOS / Windows)

```sh
cmake -S . -B build
cmake --build build
./build/bin/<project>
```

Assets are copied to `build/bin/assets/` automatically after the build.

### Web (Emscripten)

```sh
emcmake cmake -S . -B web_build
cmake --build web_build
```

Produces a self-contained `.html` that preloads all of `data/assets/` into
the wasm filesystem.

### Android

Open `android-project/` in Android Studio, or from that directory:

```sh
./gradlew assembleDebug
```

The Gradle project points `externalNativeBuild.cmake` at the root
`CMakeLists.txt` and sets `assets.srcDirs` to `../../data`, so the same
asset tree is packaged into the APK. On Android the CMake target is a shared
library named `main`, not an executable.

## Repository layout

```
CMakeLists.txt          single build for all platforms
data/assets/            images, sounds, fonts, .rml UI documents
src/
  main.cpp              SDL3 callback entry point (init/event/iterate/quit)
  core/AppContext.h     shared app state (window, renderer, audio, RmlUi)
  core/scene/           Scene base class + Manager (the reusable core)
  rmlui/                RmlUi SDL3 backend (render/system/file interfaces)
  scenes/               Splash, MainMenu (RmlUi), Game (Pong)
platform/ios/           Info.plist, launch screen, bundle icon (iOS/macOS)
platform/windows/       resources.rc (Windows executable icon)
android-project/        Gradle project for Android builds
```

## Architecture

**Entry point.** `src/main.cpp` uses SDL3's callback-style entry
(`SDL_MAIN_USE_CALLBACKS`): `SDL_AppInit` creates the window, renderer, audio
device + mixer, installs the three RmlUi interfaces and creates the single
`Rml::Context`, then starts the scene manager. `SDL_AppEvent` does two things
in order: it routes mouse/touch/keyboard input to RmlUi (touch events are
converted to RmlUi mouse moves/clicks — this is what makes the menu work on
Android), and it forwards the event to the scene system. `SDL_AppIterate`
drives `Update`/`Render`.

**Shared state.** Everything lives in the `AppContext` struct
(`src/core/AppContext.h`), passed by pointer into every scene. It is the only
shared mutable state.

**Scene system.** `core::scene::Scene` defines the lifecycle
`Init → Ready → OnEnter → Update/Render → OnExit → CleanUp`; `Manager` is a
name-keyed registry with `ChangeScene` / `RemoveScene` /
`RegisterAndInitScene`. **Transitions are driven by custom SDL events, not
direct calls**: scenes emit `SCENE_FINISHED` (or, for the menu, `START_GAME`
carrying the game mode), and `HandleScreenEvents()` in
`src/scenes/ScreenManager.h` decides what happens next. Event IDs are
registered once at startup via `SDL_RegisterEvents`.

**RmlUi integration.** `src/rmlui/` is the SDL3 backend (adapted from RmlUi's
SDL sample). Conventions to keep when adding UI:

- Fonts must be loaded with `Rml::LoadFontFace()` **before** loading any
  document (done in each scene's `Ready()`).
- Each scene loads/closes its own `Rml::ElementDocument` in `OnEnter`/`OnExit`
  and renders it via `context->Update()` + `context->Render()`.
- UI clicks are handled by `Rml::EventListener`s that push custom SDL events
  back into the queue — scenes never receive RmlUi input directly.
- `FileInterface_SDL` resolves relative paths against `SDL_GetBasePath()`,
  with an Android special case (base path is `assets:/`, so a leading
  `assets/` is stripped). Preserve this when editing file loading.
- In Debug builds the RmlUi debugger is available (toggle with F8/D).

**Audio.** SDL_mixer 3.x API: one global `MIX_Mixer` in `AppContext`; scenes
create `MIX_Track`s, load `MIX_Audio` clips, then `MIX_SetTrackAudio` +
`MIX_PlayTrack`. SFX and music live in `data/assets/sounds/`.

## Making it yours

Rename points (the project name in CMake drives the executable/library name):

- `project(...)` in `CMakeLists.txt`
- Window title and icon in `src/main.cpp`
- Bundle identifiers in `CMakeLists.txt` (iOS section)
- `android-project/app/src/main/res/values/strings.xml` and the
  `mipmap-*/ic_launcher.png` launcher icons
- `data/assets/` — replace images, sounds, fonts and the `.rml` documents

Notes:

- New source files **must be added to the explicit `SOURCES` list** in
  `CMakeLists.txt` — the project does not use globs.
- High-DPI is handled via `display_scale` (density-independent pixel ratio);
  RmlUi coordinates are physical pixels, SDL mouse events are scaled by
  `display_scale` before reaching RmlUi.
- Linux Debug builds can enable AddressSanitizer with `-DASAN=ON`.

## License

Bundled assets (images, sounds, fonts) are CC0. Code license: see `LICENSE`.
