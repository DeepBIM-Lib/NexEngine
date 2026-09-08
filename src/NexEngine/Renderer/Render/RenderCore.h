//
// Created by SherSpidey on 2026/5/28.
//

#ifndef NEXENGINE_RENDERCORE_H
#define NEXENGINE_RENDERCORE_H

#include <vector>

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>

#include <NexEngine/Renderer/Device/Backend.h>

namespace Nex
{
    struct GraphicsAdapterDetails;
    
    struct RendererCoreDesc
    {
        Render_Device_Type type;
        const char* shader_path;
    };

    struct RenderCore
    {
        Diligent::RENDER_DEVICE_TYPE device_type;

        Diligent::RefCntAutoPtr<Diligent::IEngineFactory> factory;
        Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context;
        std::vector<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> deferred_contexts;

        Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shader_factory;

        explicit RenderCore(const RendererCoreDesc& desc, GraphicsAdapterDetails& adapter_details);
        ~RenderCore();
    };
}

#endif //NEXENGINE_RENDERCORE_H
