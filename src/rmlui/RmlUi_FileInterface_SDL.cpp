#include "RmlUi_FileInterface_SDL.h"
#include <RmlUi/Core/FileInterface.h>
#include <SDL3/SDL.h>
#include <string>

Rml::FileHandle FileInterface_SDL::Open(const Rml::String &path)
{
    std::string full_path;

    // 1. Detectar si la ruta ya es absoluta o tiene un esquema VFS de SDL ("assets:/", "file://", etc.)
    bool has_scheme = (path.find(":/") != std::string::npos);
    bool is_absolute_posix = (!path.empty() && (path[0] == '/' || path[0] == '\\'));

    bool is_absolute = has_scheme || is_absolute_posix
#if defined(_WIN32)
                       || (path.size() >= 2 && path[1] == ':')
#endif
        ;

    if (is_absolute)
    {
        full_path = path;
    }
    else
    {
        const char *base = SDL_GetBasePath();
        if (base)
        {
            std::string base_str = base;
            std::string path_str = path;

            // FIX PARA ANDROID:
            // SDL_GetBasePath() devuelve "assets:/" en Android.
            // Si la ruta relativa empieza con "assets/", se lo quitamos para no duplicar y
            // buscar dentro de una subcarpeta inexistente en el APK.
            if (base_str == "assets:/" && path_str.find("assets/") == 0)
            {
                path_str = path_str.substr(7); // Recorta "assets/" del inicio
            }

            // Evitar doble barra inclinada si ambas cadenas la tienen
            if (!base_str.empty() && base_str.back() == '/' &&
                !path_str.empty() && path_str.front() == '/')
            {
                path_str = path_str.substr(1);
            }

            full_path = base_str + path_str;
        }
        else
        {
            full_path = path;
        }
    }

    SDL_IOStream *io = SDL_IOFromFile(full_path.c_str(), "rb");
    if (!io)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "RmlUi Open failed for '%s' (Resolved: '%s'): %s",
                     path.c_str(), full_path.c_str(), SDL_GetError());
    }

    return reinterpret_cast<Rml::FileHandle>(io);
}

void FileInterface_SDL::Close(Rml::FileHandle file)
{
    if (file)
        SDL_CloseIO(reinterpret_cast<SDL_IOStream *>(file));
}

size_t FileInterface_SDL::Read(void *buffer, size_t size, Rml::FileHandle file)
{
    if (!file || !buffer || size == 0)
        return 0;
    return SDL_ReadIO(reinterpret_cast<SDL_IOStream *>(file), buffer, size);
}

bool FileInterface_SDL::Seek(Rml::FileHandle file, long offset, int origin)
{
    if (!file)
        return false;

    SDL_IOWhence whence = SDL_IO_SEEK_SET;
    switch (origin)
    {
    case SEEK_SET:
        whence = SDL_IO_SEEK_SET;
        break;
    case SEEK_CUR:
        whence = SDL_IO_SEEK_CUR;
        break;
    case SEEK_END:
        whence = SDL_IO_SEEK_END;
        break;
    default:
        return false;
    }

    return SDL_SeekIO(reinterpret_cast<SDL_IOStream *>(file), offset, whence) >= 0;
}

size_t FileInterface_SDL::Tell(Rml::FileHandle file)
{
    if (!file)
        return 0;
    Sint64 pos = SDL_TellIO(reinterpret_cast<SDL_IOStream *>(file));
    return pos < 0 ? 0 : static_cast<size_t>(pos);
}

size_t FileInterface_SDL::Length(Rml::FileHandle file)
{
    if (!file)
        return 0;
    Sint64 size = SDL_GetIOSize(reinterpret_cast<SDL_IOStream *>(file));
    if (size < 0)
        return Rml::FileInterface::Length(file);
    return static_cast<size_t>(size);
}

bool FileInterface_SDL::LoadFile(const Rml::String &path, Rml::String &out_data)
{
    Rml::FileHandle handle = Open(path);
    if (!handle)
        return false;

    const size_t length = Length(handle);
    out_data.resize(length);

    size_t read_length = 0;
    if (length > 0)
        read_length = Read(&out_data[0], length, handle);

    if (length != read_length)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Could only read %zu of %zu bytes from file %s",
                    read_length, length, path.c_str());
    }

    Close(handle);
    return true;
}