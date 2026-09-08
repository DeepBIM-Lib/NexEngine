//
// Created by SherSpidey on 2026/6/25.
//

#ifndef NEXENGINE_MOVEMENT_H
#define NEXENGINE_MOVEMENT_H

#include <NexEngine/Core/Math.h>

namespace Nex
{
    struct Velocity
    {
        Vector3 data;
    };

    struct AngularVelocity
    {
        Vector3 data;
    };

    struct Acceleration
    {
        Vector3 data;
    };

    struct AngularAcceleration
    {
        Vector3 data;
    };
}

#endif //NEXENGINE_MOVEMENT_H
