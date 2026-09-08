//
// Created by SherSpidey on 2026/7/1.
//

#ifndef NEXENGINE_CAMERAMODULE_H
#define NEXENGINE_CAMERAMODULE_H

#include <flecs.h>

namespace Nex
{
    struct CameraModule
    {
        explicit CameraModule(flecs::world& world);
    };
}

#endif //NEXENGINE_CAMERAMODULE_H
