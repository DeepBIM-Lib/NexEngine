//
// Created by SherSpidey on 2026/6/30.
//

#ifndef NEXENGINE_INPUTMODULE_H
#define NEXENGINE_INPUTMODULE_H

#include <flecs.h>

namespace Nex
{
    struct InputModule
    {
        explicit InputModule(flecs::world& world);
    };
}

#endif //NEXENGINE_INPUTMODULE_H
