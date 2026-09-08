//
// Created by SherSpidey on 2026/5/23.
//

#ifndef NEXENGINE_CAMERA_H
#define NEXENGINE_CAMERA_H

#include <NexEngine/Core/Math.h>
#include <NexEngine/Core/Entity.h>
#include <NexEngine/Core/BaseType.h>

namespace Nex
{
    struct LookAt
    {
        Vector3 target;
    };

    struct Camera
    {
        float near = 0.1f;
        float far = 1000.f;
        float focal;
    };

    struct Orthographic
    {
        float height;
    };

    struct Perspective
    {
        float fov = 60.f;
    };

    enum class ClearFlag : std::uint8_t
    {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        All = Color | Depth | Stencil
    };

    struct Viewport
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 1.0f;
        float height = 1.0f;
    };

    struct RenderView
    {
        Viewport viewport;
        std::uint32_t order = 0;
        
        std::uint8_t clear_flags = raw(ClearFlag::All);
        Vector4 clear_color{0.350f, 0.350f, 0.350f, 1.0f};
        float clear_depth = 1.0f;
        std::uint8_t clear_stencil = 0;
    };

    struct CameraDesc
    {
        Camera core;
        RenderView attachment;
        Variant<
            Orthographic,
            Perspective
        > data;
    };
}

#endif //NEXENGINE_CAMERA_H
