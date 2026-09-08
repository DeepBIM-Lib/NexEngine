//
// Created by SherSpidey on 2026/6/1.
//

#ifndef NEXENGINE_ENGINECONTEXT_H
#define NEXENGINE_ENGINECONTEXT_H

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Fence.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>

namespace Diligent
{
    struct IRenderDevice;
    struct IDeviceContext;
}

namespace Nex
{
    struct EngineContext
    {
        // engine data submit serial
        uint64_t submit_serial = 0;

        // logic update index
        uint64_t logic_frame_index = 0;
        // last render-completion fence value submitted to the immediate context
        uint64_t render_frame_index = 0;

        bool retirement_signal_requested = false;

        // ResourceManager retirement is driven only by GPU-completed values from
        // this owner-thread fence
        // CPU frame counts are not completion evidence
        Diligent::RefCntAutoPtr<Diligent::IFence> render_fence;

        EngineContext() = default;

        EngineContext(Diligent::IRenderDevice* device)
        {
            Diligent::FenceDesc fence_desc;
            fence_desc.Name = "Nex Render Completion Fence";
            fence_desc.Type = Diligent::FENCE_TYPE_CPU_WAIT_ONLY;
            device->CreateFence(fence_desc, &render_fence);

            if (!render_fence)
                LOG_ERROR_AND_THROW("Failed to create the render completion fence");
        }

        EngineContext(const EngineContext&) = delete;
        EngineContext& operator=(const EngineContext&) = delete;
        EngineContext(EngineContext&&) noexcept = default;
        EngineContext& operator=(EngineContext&&) noexcept = default;
    };
}

#endif //NEXENGINE_ENGINECONTEXT_H
