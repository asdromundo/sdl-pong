#pragma once
#include <string>

namespace FileSystem
{
    // Llama a esto una sola vez al inicio de tu aplicación (ej. en main.cpp)
    void Init();

    // Úsala en cualquier parte de tu código para obtener la ruta correcta
    std::string GetAssetPath(const std::string &filepath);
}

inline std::string operator""_asset(const char *str, std::size_t)
{
    return FileSystem::GetAssetPath(str);
}