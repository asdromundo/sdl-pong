cmake_minimum_required(VERSION 3.16)

# Configuración general
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

project(pong)

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

# *** Cargando automáticamente los fuentes ***
file(GLOB_RECURSE SOURCES src/*.cpp) # Busca todos los .cpp en src y subcarpetas.

# Si estamos en Windows, incluir el archivo de recursos del ícono
if(WIN32)
    set(ICON_RESOURCE src/resources.rc)
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
target_compile_features(${EXECUTABLE_NAME} PUBLIC cxx_std_23)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
endif()

# *** SDL y librerías relacionadas (ttf, mixer, image) ***
add_subdirectory(SDL EXCLUDE_FROM_ALL)

set(SDLTTF_VENDORED ON)
add_subdirectory(SDL_ttf EXCLUDE_FROM_ALL)

set(SDLMIXER_MIDI_NATIVE OFF)
set(SDLMIXER_GME OFF)
set(SDLMIXER_WAVPACK OFF)
set(SDLMIXER_MOD OFF)
set(SDLMIXER_OPUS OFF)
set(SDLMIXER_VENDORED ON)
add_subdirectory(SDL_mixer EXCLUDE_FROM_ALL)

set(SDLIMAGE_VENDORED ON)
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_JPEG OFF)
set(SDLIMAGE_WEBP OFF)
add_subdirectory(SDL_image EXCLUDE_FROM_ALL)


target_compile_definitions(${EXECUTABLE_NAME} PRIVATE RMLUI_SDL_VERSION_MAJOR=3)
add_subdirectory(RmlUi EXCLUDE_FROM_ALL)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC 
    # SDL3_ttf::SDL3_ttf
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
    target_link_libraries(${EXECUTABLE_NAME} PUBLIC ${CF_LIB} ${CT_LIB} ${IO_LIB} ${CS_LIB} ${CG_LIB})
endif()

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)

# *** Gestión de custom commands para copiar assets ***
set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/resources)                  # Carpeta original de los assets.
set(ASSETS_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources)     # Carpeta destino en build.

# Copiar automáticamente la carpeta de assets al directorio de build.
add_custom_command(
    TARGET ${EXECUTABLE_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${ASSETS_DIR} ${ASSETS_OUTPUT_DIR}  # Copiar carpeta assets al output.
    COMMENT "Copying assets to build directory (${ASSETS_OUTPUT_DIR})."
)

# Configuración específica para diferentes plataformas.
if (EMSCRIPTEN)
    file(GLOB ASSET_FILES ${ASSETS_DIR}/*)  # Encuentra todos los archivos en assets.
    foreach(ASSET_FILE IN LISTS ASSET_FILES)
        get_filename_component(FILENAME ${ASSET_FILE} NAME)
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE "--preload-file ${ASSETS_DIR}/${FILENAME}")
    endforeach()
endif()

if(IOS)
    set(RESOURCE_FILES "src/iosLaunchScreen.storyboard;src/logo.png")
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES 
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist.in"
    XCODE_GENERATE_SCHEME TRUE
    XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER "com.asdrome.sdl3-template"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.asdrome.sdl3-template"
    RESOURCE "${RESOURCE_FILES}"
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