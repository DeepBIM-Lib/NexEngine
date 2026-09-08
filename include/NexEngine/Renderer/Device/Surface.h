//
// Created by SherSpidey on 2026/5/26.
//

#ifndef NEXENGINE_SURFACE_H
#define NEXENGINE_SURFACE_H

#include <NexEngine/Core/Math.h>
#include <NexEngine/Core/Handle.h>
#include <NexEngine/Core/BaseType.h>

namespace Nex
{
    enum class NativeWindowType : std::uint8_t
    {
        Win32 = 0,
        X11,
        Wayland
    };

    enum class ColorFormat : std::uint8_t
    {
        RGBA8_UNORM_SRGB = 0,
        RGBA8_UNORM,
        BGRA8_UNORM_SRGB,
        BGRA8_UNORM,
        RGBA16_FLOAT,
    };

    struct WindowSurface
    {
        String title{"Nex Engine"};

        NativeWindowType window_type = NativeWindowType::Win32;
        std::uintptr_t w_id;
    };

    struct OffscreenSurface
    {
        String name{"Offscreen Texture"};
    };

    struct SurfaceSource
    {
    };

    using SurfaceHandle = Handle<struct SurfaceResource>;

    struct Surface
    {
        std::uint32_t width = 1280;
        std::uint32_t height = 800;

        ColorFormat color_format = ColorFormat::RGBA8_UNORM_SRGB;
    };

    struct SurfaceDesc
    {
        Surface surface;
        Variant<
            WindowSurface,
            OffscreenSurface
        > data;
    };

    using ViewHandle = Handle<struct ViewResource>;
}

#endif //NEXENGINE_SURFACE_H
