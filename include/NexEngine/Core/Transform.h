//
// Created by SherSpidey on 2026/6/25.
//

#ifndef NEXENGINE_TRANSFORM_H
#define NEXENGINE_TRANSFORM_H

#include <NexEngine/Core/Math.h>

namespace Nex
{
    struct Position
    {
        Vector3 data{0.f, 0.f, 0.f};
    };

    struct Rotation
    {
        Vector4 data{0.f, 0.f, 0.f, 1.f};
    };

    struct Scale
    {
        Vector3 data{1.f, 1.f, 1.f};
    };

    struct Transform
    {
        Matrix4 data{IdentityMatrix4()};
    };

    struct Rotor
    {
        Vector4 base{0.f, 0.f, 0.f, 1.f};
        Vector3 data{0.f, 0.f, 0.f};
    };

    struct Kinematic
    {
    };

    struct Rigidbody
    {
    };
}

#endif //NEXENGINE_TRANSFORM_H
