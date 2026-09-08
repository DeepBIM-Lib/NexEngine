//
// Created by SherSpidey on 2026/5/26.
//

#ifndef NEXENGINE_BACKEND_H
#define NEXENGINE_BACKEND_H

#include <cstdint>

namespace Nex
{
    enum class Render_Device_Type : std::uint8_t
    {
        D3D11 = 0,
        D3D12,
        // OPENGL, // Opengl is not supported right now
        VULKAN,
    };
}

#endif //NEXENGINE_BACKEND_H
