//
// Created by SherSpidey on 2026/8/5.
//

#ifndef NEXENGINE_ENTITYUTILS_H
#define NEXENGINE_ENTITYUTILS_H

#include <flecs.h>

#include <NexEngine/Core/Entity.h>

namespace Nex
{
    inline Entity EntityCast(flecs::entity entity)
    {
        return {entity.id()};
    }
    
    inline flecs::entity EntityCast(const flecs::world& world, Entity entity)
    {
        return world.entity(entity.id);
    }
    
    template<typename T>
    bool EntityHasPair(flecs::entity entity)
    {
        return entity.target<T>().is_valid();
    }
}

#endif //NEXENGINE_ENTITYUTILS_H
