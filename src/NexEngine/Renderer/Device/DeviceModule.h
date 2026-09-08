//
// Created by SherSpidey on 2026/8/5.
//

#ifndef NEXENGINE_DEVICEMODULE_H
#define NEXENGINE_DEVICEMODULE_H

#include <flecs.h>

namespace Nex
{
    struct DeviceModule
    {
        explicit DeviceModule(flecs::world& world);
    };
}

#endif //NEXENGINE_DEVICEMODULE_H
