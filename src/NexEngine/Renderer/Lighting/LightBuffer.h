//
// Created by SherSpidey on 2026/9/3.
//

#ifndef NEXENGINE_LIGHTBUFFER_H
#define NEXENGINE_LIGHTBUFFER_H

#include <cstdint>
#include <NexEngine/Core/Math.h>

namespace Nex
{
    struct EnvironmentConstants
    {
        Vector3 ambient_color;
        float ambient_intensity;
    };
    
    struct DirectionalLightConstants
    {
        Vector3 direction;
        float intensity;

        Vector3 color;
        float padding;
    };
    
    struct LocalLightSlot
    {
        Vector3 position;
        std::uint32_t type;


        Vector3 direction;
        float range;

        Vector3 color;
        float intensity;

        float spot_inner_cosine;
        float spot_outer_cosine;
        float padding_0;
        float padding_1;
    };
}

#endif //NEXENGINE_LIGHTBUFFER_H
