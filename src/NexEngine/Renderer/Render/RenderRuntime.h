//
// Created by SherSpidey on 2026/8/25.
//

#ifndef NEXENGINE_RENDERRUNTIME_H
#define NEXENGINE_RENDERRUNTIME_H

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsTools/interface/DynamicBuffer.hpp>

#include <NexEngine/Core/IDTypes.h>

namespace Nex
{
    struct RenderViewItem;
    struct SceneLightingItem;
    struct PipelineRegistry;
    struct PersistentBufferManager;
    
    struct RenderRuntime
    {
        PersistentTableID light_table = InvalidPersistentTableID;
        PersistentTableID transform_table = InvalidPersistentTableID;

        Diligent::IRenderDevice* device = nullptr;
        PersistentBufferManager* buffer_manager = nullptr;

        Diligent::DynamicBuffer instance_buffer;
        Diligent::RefCntAutoPtr<Diligent::IBufferView> instance_view;
        std::uint32_t instance_capacity = 0;

        Diligent::RefCntAutoPtr<Diligent::IBuffer> frame_constants;

        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> frame_binding;
        Diligent::IShaderResourceVariable* batch_constants_variable = nullptr;
        
        BufferVersion instance_version = InvalidBufferVersion;
        BufferVersion transform_version = InvalidBufferVersion;
        BufferVersion light_version = InvalidBufferVersion;

        explicit RenderRuntime(Diligent::IRenderDevice*, PersistentBufferManager*);

        RenderRuntime(const RenderRuntime&) = delete;
        RenderRuntime& operator=(const RenderRuntime&) = delete;
        RenderRuntime(RenderRuntime&&) noexcept = delete;
        RenderRuntime& operator=(RenderRuntime&&) noexcept = delete;

        void UploadInstances(Diligent::IDeviceContext*, const ArrayView<const InstanceSlot>&);
        void UploadFrameConstants(Diligent::IDeviceContext*, const RenderViewItem&, const SceneLightingItem&) const;
        void UpdateFrameBinding(PipelineRegistry&);
        void CommitBatch(Diligent::IDeviceContext*, std::uint32_t) const;
    };
}

#endif //NEXENGINE_RENDERRUNTIME_H
