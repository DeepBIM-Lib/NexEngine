//
// Created by SherSpidey on 2026/6/15.
//

#ifndef NEXENGINE_TEXTURE_H
#define NEXENGINE_TEXTURE_H

namespace Nex
{
    enum class TextureColorSpace : bool
    {
        Linear = false,
        SRGB = true
    };
    
    enum class TextureFallback : std::uint8_t
    {
        WhiteSRGB = 0,
        WhiteLinear,
        Black,
        FlatNormal,
        Transparent,
        Count
    };
}

#endif //NEXENGINE_TEXTURE_H
