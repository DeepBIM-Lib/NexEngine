//
// Created by SherSpidey on 2026/7/2.
//

#ifndef NEXENGINE_FLYCONTROLLER_H
#define NEXENGINE_FLYCONTROLLER_H

#include <NexEngine/Core/Math.h>

namespace Nex
{
    struct FlyController
    {
        float min_speed = 0.1f;
        float max_speed = 40.f;
        float move_speed = 5.f;
        float scroll_speed_scale = 1.5f;

        float sprint_scale = 5.f;
        float acceleration_smooth = 6.f;
        float deceleration_smooth = 18.f;

        float angular_max_speed = 1.5f;
        float angular_acceleration_smooth = 5.f;
        float angular_deceleration_smooth = 15.f;

        float mouse_sensitivity = 0.003f;

        float camera_pitch_limit = PI_DIV_2 - 0.05f;
    };
}

#endif //NEXENGINE_FLYCONTROLLER_H
