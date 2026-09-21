cmake_minimum_required(VERSION 3.22)

set(APP_NAME "pong")
set(APP_DISPLAY_NAME "Pong")
set(APP_DESCRIPTION "A simple multiplatform Pong game")
set(APP_AUTHOR "Asdrome Labs")

project(${APP_NAME})

# 1. General Setup
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Install path" FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
endif()

# 3DS, Web, and certain Apple builds require strictly static builds
if ((APPLE AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin") OR EMSCRIPTEN OR CMAKE_SYSTEM_NAME MATCHES "3DS")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(SDL_SHARED OFF CACHE INTERNAL "")
else()
    set(SDL_SHARED ON CACHE INTERNAL "")
endif()

# 2. Target Setup
set(EXECUTABLE_NAME ${PROJECT_NAME})

if (ANDROID)
    set(EXECUTABLE_NAME main)
    add_library(${EXECUTABLE_NAME} SHARED)
else()
    add_executable(${EXECUTABLE_NAME})
endif()

# 3. Sources
set(SOURCES
    src/main.cpp
    src/core/AppContext.h
    src/core/scene/Events.h
    src/core/scene/Manager.cpp
    src/core/scene/Manager.h
    src/core/scene/Scene.h
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
    src/scenes/MainMenuScene.cpp
    src/scenes/MainMenuScene.h
    src/scenes/ScreenManager.h
    src/scenes/SplashScene.cpp
    src/scenes/SplashScene.h
)

if(WIN32)
    target_sources(${EXECUTABLE_NAME} PRIVATE platform/windows/resources.rc)
elseif(APPLE)
    target_sources(${EXECUTABLE_NAME} PRIVATE platform/ios/logo.png)
endif()

target_sources(${EXECUTABLE_NAME} PRIVATE ${SOURCES})
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

# 4. Compiler Options
target_compile_features(${EXECUTABLE_NAME} PRIVATE cxx_std_23)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -Wpedantic)

# ASan (Disabled on 3DS - unsupported by devkitARM)
set(ASAN ON CACHE BOOL "Enable AddressSanitizer")
if (ASAN AND NOT CMAKE_SYSTEM_NAME MATCHES "3DS")
    target_compile_options(${EXECUTABLE_NAME} PRIVATE 
        $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer>
    )
    target_link_options(${EXECUTABLE_NAME} PRIVATE 
        $<$<CONFIG:Debug>:-fsanitize=address>
    )
endif()

if(EMSCRIPTEN)
    target_link_options(${EXECUTABLE_NAME} PRIVATE
        "-sUSE_WebGL2=1"
        "-sALLOW_MEMORY_GROWTH=1"
    )
endif()

# 5. Dependencies (Dynamic vs Static Fetching)
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Speed up reconfiguration")

# Modificado para forzar la descarga estática en entornos cruzados (3DS, Android, Web)
function(fetch_or_find name version repo tag)
    if(CMAKE_SYSTEM_NAME MATCHES "3DS" OR EMSCRIPTEN OR ANDROID)
        set(${name}_FOUND FALSE)
    else()
        find_package(${name} ${version} QUIET)
    endif()
    
    if(NOT ${name}_FOUND)
        FetchContent_Declare(${name}
            GIT_REPOSITORY ${repo}
            GIT_TAG ${tag}
            GIT_SHALLOW ON
        )
        FetchContent_MakeAvailable(${name})
    endif()
endfunction()

fetch_or_find(SDL3 3.4.14 https://github.com/libsdl-org/SDL.git release-3.4.16)

set(SDLMIXER_MIDI_NATIVE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDLMIXER_FLAC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VENDORED ON CACHE BOOL "" FORCE)

if(CMAKE_SYSTEM_NAME MATCHES "3DS" OR EMSCRIPTEN OR ANDROID)
    set(SDLMIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VORBIS_VORBISFILE_SHARED OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MP3_MPG123_SHARED OFF CACHE BOOL "" FORCE)
    
    # Prevenir el mismo problema para SDL3_image
    set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "3DS")
    set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MP3_MINIMP3 ON CACHE BOOL "" FORCE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "3DS" OR EMSCRIPTEN OR ANDROID)
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
else()
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
endif()
fetch_or_find(SDL3_mixer 3.2.4 https://github.com/libsdl-org/SDL_mixer.git release-3.2.4)
fetch_or_find(SDL3_image 3.4.4 https://github.com/libsdl-org/SDL_image.git release-3.4.4)

# FreeType override for constrained environments
if(CMAKE_SYSTEM_NAME MATCHES "3DS" OR EMSCRIPTEN OR ANDROID)
    set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

    FetchContent_Declare(freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG VER-2-14-3
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(freetype)

    if(TARGET freetype AND NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
    endif()

    file(WRITE "${CMAKE_BINARY_DIR}/cmake_mocks/FindFreetype.cmake" "
        set(FREETYPE_FOUND TRUE)
        set(Freetype_FOUND TRUE)
    ")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/cmake_mocks")
endif()

fetch_or_find(RmlUi 6.2 https://github.com/mikke89/RmlUi.git 6.2)

# 6. Linking
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE RMLUI_SDL_VERSION_MAJOR=3)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE SDL_MAIN_USE_CALLBACKS)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    SDL3_mixer::SDL3_mixer
    SDL3_image::SDL3_image
    SDL3::SDL3
    RmlUi::RmlUi
)

if (CMAKE_SYSTEM_NAME MATCHES "3DS")
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE citro3d ctru m)
endif()

if(APPLE AND NOT BUILD_SHARED_LIBS)
    find_library(IO_LIB ImageIO REQUIRED)
    find_library(CS_LIB CoreServices REQUIRED)
    find_library(CT_LIB CoreText REQUIRED)
    find_library(CG_LIB CoreGraphics REQUIRED)
    find_library(CF_LIB CoreFoundation REQUIRED)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${CF_LIB} ${CT_LIB} ${IO_LIB} ${CS_LIB} ${CG_LIB})
endif()

if(MINGW)
    target_link_options(${EXECUTABLE_NAME} PRIVATE "-mwindows")
endif()

# 7. Assets and Packaging
set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/assets")
set(ASSETS_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

if(EXISTS "${ASSETS_DIR}")
    add_custom_command(
        TARGET ${EXECUTABLE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_DIR}" "${ASSETS_OUTPUT_DIR}"
    )
endif()

if (EMSCRIPTEN)
    file(GLOB ASSET_FILES RELATIVE "${ASSETS_DIR}" CONFIGURE_DEPENDS "${ASSETS_DIR}/*")
    foreach(ASSET_FILE IN LISTS ASSET_FILES)
        target_link_options(${EXECUTABLE_NAME} PRIVATE 
            "--preload-file=${ASSETS_DIR}/${ASSET_FILE}@assets/${ASSET_FILE}"
        )
    endforeach()
endif()

# 3DS ROMFS Packaging
if (CMAKE_SYSTEM_NAME MATCHES "3DS")
    # En lugar de empaquetar "data/assets" como la raíz,
    # empaquetamos el directorio de salida (bin/) para preservar la subcarpeta "assets/"
    set(ROMFS_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    
    set(ROMFS_FLAGS "")
    if(EXISTS "${ROMFS_DIR}/assets")
        set(ROMFS_FLAGS "--romfs=${ROMFS_DIR}")
    endif()
    
    add_custom_command(
        TARGET ${EXECUTABLE_NAME} POST_BUILD
        COMMAND smdhtool --create "${APP_DISPLAY_NAME}" "${APP_DESCRIPTION}" "${APP_AUTHOR}" "${ASSETS_DIR}/pong_48x48.png" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE_NAME}.smdh"
        
        # El comando usa el ROMFS_DIR que contiene la carpeta 'assets'
        COMMAND 3dsxtool $<TARGET_FILE:${EXECUTABLE_NAME}> "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE_NAME}.3dsx" ${ROMFS_FLAGS} --smdh="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE_NAME}.smdh"
        COMMENT "Packaging ${EXECUTABLE_NAME}.3dsx with SMDH metadata and mounting ROMFS..."
    )
endif()

set(RESOURCE_FILES "")
if(IOS)
    set(RESOURCE_FILES "platform/ios/iosLaunchScreen.storyboard;platform/ios/logo.png")
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/Info.plist.in"
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
    set(CPACK_GENERATOR "DragNDrop")
    include(CPack)
endif()