//
// Created by SherSpidey on 2026/5/26.
//

#ifndef NEXENGINE_CAPABILITY_H
#define NEXENGINE_CAPABILITY_H

#include <NexEngine/Core/BaseType.h>
#include <NexEngine/Renderer/Device/Backend.h>

namespace Nex
{
    struct ImmediateEngineDesc
    {
    };

    struct DeferredEngineDesc
    {
    };

    struct EngineCreateDesc
    {
        String scene_script;
        String shader_search_path;
        Render_Device_Type device_type = Render_Device_Type::D3D11;

        Variant<
            ImmediateEngineDesc,
            DeferredEngineDesc
        > description;
    };
}

#endif //NEXENGINE_CAPABILITY_H
