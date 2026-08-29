cmake_minimum_required(VERSION 3.22...4.2)
project(pong)

# General setup
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Install path" FORCE)
endif()

if(CMAKE_TOOLCHAIN_FILE MATCHES "emscripten|Emscripten")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
endif()

if ((APPLE AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin") OR EMSCRIPTEN)
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(SDL_SHARED OFF)
else()
    set(SDL_SHARED ON)
endif()

# Target setup
set(EXECUTABLE_NAME ${PROJECT_NAME})

if (ANDROID)
    set(EXECUTABLE_NAME main)
    add_library(${EXECUTABLE_NAME} SHARED)
else()
    add_executable(${EXECUTABLE_NAME})
endif()

# Sources
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
endif()

if(APPLE)
    target_sources(${EXECUTABLE_NAME} PRIVATE platform/ios/logo.png)
endif()

target_sources(${EXECUTABLE_NAME} PRIVATE ${SOURCES})
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

# Compiler options
target_compile_features(${EXECUTABLE_NAME} PRIVATE cxx_std_23)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -Wpedantic)

# ASan
set(ASAN OFF CACHE BOOL "Enable AddressSanitizer")
if (ASAN)
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

# Dependencies
include(FetchContent)
# Android CMake is currently 3.22, which doesn't support
# FIND_PACKAGE_ARGS, this is a workaround for the functionality.
function(fetch_or_find name version repo tag)
    find_package(${name} ${version} QUIET)
    if(NOT ${name}_FOUND)
        FetchContent_Declare(${name}
            GIT_REPOSITORY ${repo}
            GIT_TAG ${tag}
            GIT_SHALLOW ON
        )
        FetchContent_MakeAvailable(${name})
    endif()
endfunction()

fetch_or_find(SDL3 3.4.14 https://github.com/libsdl-org/SDL.git release-3.4.14)

set(SDLMIXER_MIDI_NATIVE OFF)
set(SDLMIXER_GME OFF)
set(SDLMIXER_WAVPACK OFF)
set(SDLMIXER_MOD OFF)
set(SDLMIXER_OPUS OFF)
set(SDLMIXER_FLAC OFF)
set(SDLMIXER_VENDORED ON)
fetch_or_find(SDL3_mixer 3.2.4 https://github.com/libsdl-org/SDL_mixer.git release-3.2.4)

set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_VENDORED ON)
fetch_or_find(SDL3_image 3.4.4 https://github.com/libsdl-org/SDL_image.git release-3.4.4)

# FreeType override for Android/Web
if(ANDROID OR EMSCRIPTEN)
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

# Linking
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE RMLUI_SDL_VERSION_MAJOR=3)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE SDL_MAIN_USE_CALLBACKS)

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

if(MINGW)
    target_link_options(${EXECUTABLE_NAME} PRIVATE "-mwindows")
endif()

# Assets
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

# Packaging
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