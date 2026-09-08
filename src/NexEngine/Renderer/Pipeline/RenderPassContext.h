//
// Created by SherSpidey on 2026/8/31.
//

#ifndef NEXENGINE_RENDERPASSCONTEXT_H
#define NEXENGINE_RENDERPASSCONTEXT_H

#include <NexEngine/Renderer/Render/RenderFrame.h>
#include <NexEngine/Renderer/Render/RenderRuntime.h>
#include <NexEngine/Renderer/Pipeline/PipelineRegistry.h>
#include <NexEngine/Renderer/Material/MaterialBindingCache.h>
#include <NexEngine/Utils/MaterialManager.h>
#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

namespace Nex
{
    struct RenderPassContext
    {
        Diligent::IRenderDevice* device = nullptr;
        Diligent::IDeviceContext* context = nullptr;

        RenderRuntime* render_runtime = nullptr;
        PipelineRegistry* pipeline_registry = nullptr;
        MaterialBindingCache* material_cache = nullptr;
        PersistentBufferManager* buffer_manager = nullptr;
        MaterialManager* material_manager = nullptr;
        ResourceManager* resource_manager = nullptr;
        
        const RenderFrame* render_frame = nullptr;
        const RenderViewItem* render_view = nullptr;
        
        Diligent::ITextureView* render_target_view = nullptr;
        Diligent::ITextureView* depth_stencil_view = nullptr;
        Diligent::TEXTURE_FORMAT color_format = Diligent::TEX_FORMAT_UNKNOWN;
        Diligent::TEXTURE_FORMAT depth_format = Diligent::TEX_FORMAT_UNKNOWN;
    };
}

#endif //NEXENGINE_RENDERPASSCONTEXT_H
