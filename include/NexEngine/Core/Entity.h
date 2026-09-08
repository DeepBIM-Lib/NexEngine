//
// Created by SherSpidey on 2026/8/5.
//

#ifndef NEXENGINE_ENTITY_H
#define NEXENGINE_ENTITY_H

#include <NexEngine/Core/Hash.h>

namespace Nex
{
    struct Entity
    {
        std::uint64_t id = 0;
    };

    inline bool operator==(const Entity& lhs, const Entity& rhs)
    {
        return lhs.id == rhs.id;
    }
}

template <>
struct std::hash<Nex::Entity>
{
    std::size_t operator()(const Nex::Entity& entity) const noexcept
    {
        return Nex::ComputeHash(entity.id);
    }
};

#endif //NEXENGINE_ENTITY_H
