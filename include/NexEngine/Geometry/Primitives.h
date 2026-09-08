//
// Created by SherSpidey on 2026/7/8.
//

#ifndef NEXENGINE_PRIMITIVES_H
#define NEXENGINE_PRIMITIVES_H

#include <cstdint>
#include <NexEngine/Core/Math.h>

namespace Nex
{
    enum class PrimitiveType : std::uint16_t
    {
        Cube = 0,
        QuadSphere = 1,

        // UVSphere,
        // IcoSphere,
        // Cylinder,
        // Plane,
        // ...

        Count
    };

    struct Cube
    {
        float x = 1.f;
        float y = 1.f;
        float z = 1.f;
    };

    // default sphere type
    struct Sphere
    {
        float radius = 1.f;
    };

    struct QuadSphere
    {
        float radius = 1.f;
    };

    struct PrimitiveSource
    {
    };
    
    struct PrimitiveScale
    {
        Vector3 scale{1.f, 1.f, 1.f};
    };
    
    constexpr bool IsAffine(PrimitiveType type) noexcept
    {
        switch (type)
        {
        case PrimitiveType::Cube:
        case PrimitiveType::QuadSphere:
            return true;
        
        case PrimitiveType::Count:
        default:
            return false;
        }
    }
}

#endif //NEXENGINE_PRIMITIVES_H
