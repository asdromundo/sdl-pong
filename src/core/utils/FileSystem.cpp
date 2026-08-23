#include "FileSystem.h"
#include <SDL3/SDL.h>
#include <stdexcept>

namespace FileSystem
{
    static std::string s_BasePath;

    void Init()
    {
        const char *basePath = SDL_GetBasePath();
        if (basePath)
        {
            s_BasePath = basePath;
        }
        else
        {
            // Manejo de error si SDL falla por alguna razón
            s_BasePath = "";
        }
    }

    std::string GetAssetPath(const std::string &filepath)
    {
        return s_BasePath + filepath;
    }

}