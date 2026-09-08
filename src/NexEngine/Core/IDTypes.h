//
// Created by SherSpidey on 2026/8/23.
//

#ifndef NEXENGINE_IDTYPES_H
#define NEXENGINE_IDTYPES_H

#include <cstdint>
#include <string>

namespace Nex
{
    using FramePassID = std::string;
    using FrameResourceID = std::string;
    
    using TechniqueID = std::uint32_t;
    inline constexpr TechniqueID InvalidTechniqueID = 0;
    
    using PersistentTableID = std::uint32_t;
    inline constexpr PersistentTableID InvalidPersistentTableID = 0;

    using PersistentSlotIndex = std::uint32_t;
    inline constexpr PersistentSlotIndex InvalidPersistentSlotIndex = 0xFFFFFFFFu;
    
    using BufferVersion = std::uint32_t;
    inline constexpr BufferVersion InvalidBufferVersion = 0xFFFFFFFFu;
}

#endif //NEXENGINE_IDTYPES_H
