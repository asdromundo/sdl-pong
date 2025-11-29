#ifndef CORE_UTILS_TEXTURE_H
#define CORE_UTILS_TEXTURE_H

#include <SDL3/SDL_rect.h>

namespace core::utils::image {

    enum class Pivot {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    enum class ScaleMode {
        Fit,     // Mantiene aspecto, contenido visible (puede dejar márgenes)
        Fill,    // Mantiene aspecto, rellena área (puede recortar)
        Stretch  // No mantiene aspecto, deforma para ocupar todo
    };

    SDL_FRect GetImageRect(
        int targetWidth,
        int targetHeight,
        float maxWidthRatio = 1.0f,
        float maxHeightRatio = 1.0f,
        float aspectRatio = 1.0f,
        Pivot pivot = Pivot::Center,
        ScaleMode scaleMode = ScaleMode::Fit
    );

    // Funciones convenientes comunes
    SDL_FRect GetImageRect_CenterFit(int targetWidth, int targetHeight, float aspectRatio = 1.0f);
    SDL_FRect GetImageRect_CenterFill(int targetWidth, int targetHeight, float aspectRatio = 1.0f);
    SDL_FRect GetImageRect_CenterStretch(int targetWidth, int targetHeight);

} // namespace core::utils::image

#endif // CORE_UTILS_TEXTURE_H
