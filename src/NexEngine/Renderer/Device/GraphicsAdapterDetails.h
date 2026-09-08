//
// Created by SherSpidey on 2026/6/1.
//

#ifndef NEXENGINE_GRAPHICSADAPTERDETAILS_H
#define NEXENGINE_GRAPHICSADAPTERDETAILS_H

#include <vector>
#include <DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h>

namespace Nex
{
    struct GraphicsAdapterDetails
    {
        Diligent::Uint32 adapter_id = Diligent::DEFAULT_ADAPTER_ID;
        Diligent::ADAPTER_TYPE adapter_type = Diligent::ADAPTER_TYPE_UNKNOWN;

        Diligent::GraphicsAdapterInfo adapter_info;
        std::vector<Diligent::DisplayModeAttribs> display_modes;
    };
}

#endif //NEXENGINE_GRAPHICSADAPTERDETAILS_H
