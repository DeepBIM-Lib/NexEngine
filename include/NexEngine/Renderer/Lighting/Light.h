//
// Created by SherSpidey on 2026/9/1.
//

#ifndef NEXENGINE_LIGHT_H
#define NEXENGINE_LIGHT_H

#include <NexEngine/Core/Math.h>

namespace Nex
{
    struct EnvironmentLight
    {
        Vector3 ambient_color{0.12f, 0.15f, 0.20f};
        float ambient_intensity = 1.0f;
    };

    struct DirectionalLight
    {
        Vector3 direction{-0.4082f, -0.8165f, -0.4082f};
        Vector3 color{1.0f, 0.96f, 0.88f};

        float intensity = 1.5f;
    };

    struct PointLight
    {
        Vector3 color{1.0f, 1.0f, 1.0f};
        float intensity = 1.5f;
        
        float range = 10.0f;
    };

    struct SpotLight
    {
        Vector3 color{1.0f, 1.0f, 1.0f};
        float intensity = 2.0f;
        float range = 10.0f;
        
        float inner_angle = 0.349066f;
        float outer_angle = 0.610865f;
    };
}

#endif // NEXENGINE_LIGHT_H
