//
// Created by SherSpidey on 2026/6/5.
//

#ifndef NEXENGINE_PRIMITIVESHAPE_H
#define NEXENGINE_PRIMITIVESHAPE_H

#include <NexEngine/Core/Math.h>
#include <NexEngine/Geometry/MeshData.h>

namespace Nex
{
    struct Edge
    {
        uint32_t a;
        uint32_t b;
    };

    struct PrimitiveShape
    {
        MeshData data;
    };

    struct StandardShape : PrimitiveShape
    {
    };

    struct ParametricShape : PrimitiveShape
    {
    };
}

#endif //NEXENGINE_PRIMITIVESHAPE_H
