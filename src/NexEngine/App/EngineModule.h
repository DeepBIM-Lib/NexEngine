//
// Created by SherSpidey on 2026/6/24.
//

#ifndef NEXENGINE_MODULE_H
#define NEXENGINE_MODULE_H

#include <flecs.h>

namespace Nex
{
    struct EngineModule
    {
        explicit EngineModule(flecs::world& world);
    };
}

#endif //NEXENGINE_MODULE_H
