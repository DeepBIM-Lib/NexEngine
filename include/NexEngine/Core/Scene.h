//
// Created by SherSpidey on 2026/9/3.
//

#ifndef NEXENGINE_SCENE_H
#define NEXENGINE_SCENE_H

#include <cstdint>

namespace Nex
{
    inline constexpr std::uint32_t MaxActiveScenes = 8;
    
    struct Scene
    {
        std::uint32_t index = 0;
    };
}

#endif //NEXENGINE_SCENE_H
