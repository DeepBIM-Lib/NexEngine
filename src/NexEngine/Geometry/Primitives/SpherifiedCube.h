//
// Created by SherSpidey on 2026/6/4.
//

#ifndef NEXENGINE_CUBE_H
#define NEXENGINE_CUBE_H

#include <NexEngine/Geometry/Primitives/PrimitiveShape.h>

namespace Nex::Primitives
{
    struct Cube : StandardShape
    {
    };

    struct QuadSphere : StandardShape
    {
    };

    void Generate(Cube& cube, float size = 1.f);
    void Generate(QuadSphere& sphere, float radius = 1.f);
}

#endif //NEXENGINE_CUBE_H
