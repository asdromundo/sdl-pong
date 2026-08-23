cmake_minimum_required(VERSION 3.22)

# Configuración general
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

project(pong)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
endif()

if ((APPLE AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin") OR EMSCRIPTEN)
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(SDL_SHARED OFF)
else()
    set(SDL_SHARED ON)
endif()

if(MSVC)
    if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
        add_definitions(/MP)    # Paralelizar compilación en MSVC cuando Ninja no se usa.
    endif()
endif()

set(EXECUTABLE_NAME ${PROJECT_NAME})

# Crear ejecutable específico basado en la plataforma.
if (ANDROID)
    set(EXECUTABLE_NAME main)
    add_library(${EXECUTABLE_NAME} SHARED)
else()
    add_executable(${EXECUTABLE_NAME})
endif()

# *** Cargando explícitamente los fuentes ***
set(SOURCES
    src/main.cpp

    src/core/AppContext.h
    src/core/Renderer.h

    src/core/scene/Events.h
    src/core/scene/Manager.cpp
    src/core/scene/Manager.h
    src/core/scene/Scene.h
    src/core/scene/StateMachine.h

    src/core/utils/image/Texture.cpp
    src/core/utils/image/Texture.h    

    src/game/Components.h
    src/game/Mode.h

    src/rmlui/RmlUi_Platform_SDL.cpp
    src/rmlui/RmlUi_Platform_SDL.h
    src/rmlui/RmlUi_Renderer_SDL.cpp
    src/rmlui/RmlUi_Renderer_SDL.h
    src/rmlui/RmlUi_FileInterface_SDL.cpp
    src/rmlui/RmlUi_FileInterface_SDL.h

    src/scenes/GameScene.cpp
    src/scenes/GameScene.h
    src/scenes/IntroScene.cpp
    src/scenes/IntroScene.h
    src/scenes/MainMenuScene.cpp
    src/scenes/MainMenuScene.h
    src/scenes/ScreenManager.h
    src/scenes/SplashScene.cpp
    src/scenes/SplashScene.h
)

# Si estamos en Windows, incluir el archivo de recursos del ícono
if(WIN32)
    set(ICON_RESOURCE src/assets.rc)
    target_sources(${EXECUTABLE_NAME} PRIVATE ${ICON_RESOURCE})
endif()

target_sources(${EXECUTABLE_NAME} PRIVATE ${SOURCES})

# *** Incluir directorios ***
target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/src  # Incluye todos los archivos .h en src y subcarpetas.
)

# Configuración para Apple: Archivos y recursos específicos.
if(APPLE)
    target_sources("${EXECUTABLE_NAME}" PRIVATE "src/logo.png")
endif()

# Requerimientos del estándar de C++
target_compile_features(${EXECUTABLE_NAME} PRIVATE cxx_std_23)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES CXX_EXTENSIONS OFF)

# Opciones de compilación
if(MSVC)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE /W4)
else()
    target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Verificacion de memoria
if (LINUX)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE 
    $<$<CONFIG:Debug>:-fsanitize=address -g -fno-omit-frame-pointer>
    )

    target_link_options(${EXECUTABLE_NAME} PRIVATE 
    $<$<CONFIG:Debug>:-fsanitize=address>
    )
endif()

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
endif()

# *** SDL y librerías relacionadas - Con fallback a FetchContent ***
include(FetchContent)

set(SDL_X11 OFF CACHE INTERNAL "")

# Helper para evitar repetir el patrón find_package + FetchContent
function(fetch_or_find name version repo tag)
    find_package(${name} ${version} QUIET)
    if(NOT ${name}_FOUND)
        message(STATUS "${name} no encontrado, descargando (tag: ${tag})...")
        FetchContent_Declare(${name}
            GIT_REPOSITORY ${repo}
            GIT_TAG ${tag}
            GIT_SHALLOW ON
        )
        FetchContent_MakeAvailable(${name})
    endif()
endfunction()

# SDL3 -> apuntando a la rama "main" en vez de un release fijo
fetch_or_find(SDL3 3.4.14 https://github.com/libsdl-org/SDL.git main)

# SDL_mixer
set(SDLMIXER_MIDI_NATIVE OFF)
set(SDLMIXER_GME OFF)
set(SDLMIXER_WAVPACK OFF)
set(SDLMIXER_MOD OFF)
set(SDLMIXER_OPUS OFF)
set(SDLMIXER_FLAC OFF)
set(SDLMIXER_VENDORED ON)
fetch_or_find(SDL3_mixer 3.2.4 https://github.com/libsdl-org/SDL_mixer.git release-3.2.0)

# SDL_image
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_VENDORED ON)
fetch_or_find(SDL3_image 3.4.4 https://github.com/libsdl-org/SDL_image.git release-3.4.4)

# --- Solución para FreeType en Android ---
if(ANDROID)
    message(STATUS "Android detectado: Descargando FreeType desde código fuente...")

    # Desactivamos dependencias extra de FreeType para que compile directo y sin errores
    set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable Zlib" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable BZip2" FORCE)
    set(FT_DISABLE_PNG ON CACHE BOOL "Disable PNG" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli" FORCE)

    FetchContent_Declare(freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG VER-2-13-2 # Versión estable
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(freetype)

    # RmlUi buscará el target 'Freetype::Freetype'. Lo creamos como alias.
    if(TARGET freetype AND NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
    endif()

    # Creamos un archivo falso FindFreetype.cmake temporal.
    # Esto evita que RmlUi busque en el sistema host y aborte la configuración.
    file(WRITE "${CMAKE_BINARY_DIR}/cmake_mocks/FindFreetype.cmake" "
        set(FREETYPE_FOUND TRUE)
        set(Freetype_FOUND TRUE)
    ")
    
    # Le decimos a CMake que busque nuestros módulos falsos primero
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/cmake_mocks")
endif()
# ------------------------------------------

# RmlUi (Ahora encontrará el FreeType que acabamos de compilar)
fetch_or_find(RmlUi 6.2 https://github.com/mikke89/RmlUi.git 6.2)

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE RMLUI_SDL_VERSION_MAJOR=3)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    SDL3_mixer::SDL3_mixer
    SDL3_image::SDL3_image
    SDL3::SDL3
    RmlUi::RmlUi
)

if(APPLE AND NOT BUILD_SHARED_LIBS)
    find_library(IO_LIB ImageIO REQUIRED)
    find_library(CS_LIB CoreServices REQUIRED)
    find_library(CT_LIB CoreText REQUIRED)
    find_library(CG_LIB CoreGraphics REQUIRED)
    find_library(CF_LIB CoreFoundation REQUIRED)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${CF_LIB} ${CT_LIB} ${IO_LIB} ${CS_LIB} ${CG_LIB})
endif()

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE SDL_MAIN_USE_CALLBACKS)

# *** Gestión de custom commands para copiar assets ***
set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data/assets)              # Carpeta original de los assets.
set(ASSETS_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)     # Carpeta destino en build.

# Copiar automáticamente la carpeta de assets al directorio de build.
if(EXISTS ${ASSETS_DIR})
    add_custom_command(
        TARGET ${EXECUTABLE_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${ASSETS_DIR} ${ASSETS_OUTPUT_DIR}  # Copiar carpeta assets al output.
        COMMENT "Copying assets to build directory (${ASSETS_OUTPUT_DIR})."
    )
else()
    message(WARNING "Assets directory not found: ${ASSETS_DIR}")
endif()

# Configuración específica para diferentes plataformas.
if (EMSCRIPTEN)
    file(GLOB ASSET_FILES RELATIVE "${ASSETS_DIR}" CONFIGURE_DEPENDS "${ASSETS_DIR}/*")
    foreach(ASSET_FILE IN LISTS ASSET_FILES)
        get_filename_component(FILENAME ${ASSET_FILE} NAME)
        target_link_options(${EXECUTABLE_NAME} PRIVATE "--preload-file=${ASSETS_DIR}/${FILENAME}")
    endforeach()
endif()

set(RESOURCE_FILES "")
if(IOS)
    set(RESOURCE_FILES "src/iosLaunchScreen.storyboard;src/logo.png")
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist.in"
    XCODE_GENERATE_SCHEME TRUE
    XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER "com.asdrome.pong"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.asdrome.pong"
    RESOURCE "${RESOURCE_FILES}"
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${EXECUTABLE_NAME}")

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    install(TARGETS ${EXECUTABLE_NAME}
        BUNDLE DESTINATION ./install COMPONENT Runtime
        RUNTIME DESTINATION ./install/bin COMPONENT Runtime
    )
    set(DEP_DIR "${CMAKE_BINARY_DIR}")
    INSTALL(CODE
        "include(BundleUtilities)
        fixup_bundle(\"$<TARGET_BUNDLE_DIR:${EXECUTABLE_NAME}>\" \"\" \"${DEP_DIR}\")
        "
    )
    set(CPACK_GENERATOR "DragNDrop")
    include(CPack)
endif()

if(MINGW)
    target_link_options(${EXECUTABLE_NAME} PRIVATE "-mwindows")
endif()
