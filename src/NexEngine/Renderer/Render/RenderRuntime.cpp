//
// Created by SherSpidey on 2026/8/25.
//

#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp>

#include <NexEngine/Renderer/Render/RenderBuffer.h>
#include <NexEngine/Renderer/Render/RenderFrame.h>

#include <NexEngine/Renderer/Pipeline/PipelineRegistry.h>

#include <NexEngine/Utils/DiligentUtils.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

#include <NexEngine/Renderer/Render/RenderRuntime.h>

using namespace Nex;
using namespace Diligent;

namespace
{
    constexpr std::uint32_t InitialInstanceCapacity = 1024;

    BufferDesc MakeInstanceBufferDesc()
    {
        BufferDesc desc;
        desc.Name = ShaderSlots::Instances;
        desc.Size = sizeof(InstanceSlot) * InitialInstanceCapacity;
        desc.Usage = USAGE_DYNAMIC;
        desc.BindFlags = BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = CPU_ACCESS_WRITE;
        desc.Mode = BUFFER_MODE_STRUCTURED;
        desc.ElementByteStride = sizeof(InstanceSlot);
        return desc;
    }
}

RenderRuntime::RenderRuntime(IRenderDevice* render_device, PersistentBufferManager* persistent_buffer_manager)
    : device{render_device},
      buffer_manager{persistent_buffer_manager},
      instance_buffer{render_device, DynamicBufferCreateInfo{MakeInstanceBufferDesc()}}

{
    if (!render_device || !persistent_buffer_manager) return;

    transform_table = buffer_manager->Declare(sizeof(TransformSlot), 256, "g_transform_table");
    light_table = buffer_manager->Declare(sizeof(LocalLightSlot), 256, "g_light_table");
    
    instance_capacity = InitialInstanceCapacity;
    if (const auto buffer = instance_buffer.GetBuffer())
        instance_view = RefCntAutoPtr(buffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

    CreateUniformBuffer(
        device, sizeof(ForwardFrameConstants), ShaderSlots::FrameConstants, frame_constants);
}

void RenderRuntime::UploadInstances(IDeviceContext* context, const ArrayView<const InstanceSlot>& data)
{
    if (!context || data.empty()) return;

    // expand
    if (data.size > instance_capacity)
    {
        auto capacity = instance_capacity > 0 ? instance_capacity : InitialInstanceCapacity;
        while (capacity < data.size) capacity *= 2;

        instance_buffer.Resize(device,
                               context,
                               static_cast<Uint64>(capacity) * sizeof(InstanceSlot),
                               true);

        if (const auto buffer = instance_buffer.GetBuffer())
        {
            instance_view = RefCntAutoPtr(buffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
            instance_capacity = capacity;
        }
    }

    auto buffer = instance_buffer.GetBuffer();
    if (!buffer) return;

    MapHelper<InstanceSlot> mapped(context, buffer, MAP_WRITE, MAP_FLAG_DISCARD);
    if (!mapped) return;

    std::memcpy(mapped, data.data, data.size * sizeof(InstanceSlot));
}

void RenderRuntime::UploadFrameConstants(IDeviceContext* context,
                                         const RenderViewItem& render_view,
                                         const SceneLightingItem& scene_lighting) const
{
    if (!context || !frame_constants) return;

    ForwardFrameConstants constants{};
    auto& camera = constants.camera;
    auto& frame = constants.frame;
    auto& environment = constants.environment;
    auto& sun = constants.sun;

    camera.projection = render_view.projection_matrix;
    camera.view = render_view.view_matrix;
    camera.position = render_view.camera_position;

    frame.local_light_count = scene_lighting.local_light_count;
    
    environment = scene_lighting.environment;
    sun = scene_lighting.sun;

    MapHelper<ForwardFrameConstants> mapped(context, frame_constants,
                                            MAP_WRITE, MAP_FLAG_DISCARD);
    if (!mapped) return;

    *mapped = constants;
}

void RenderRuntime::UpdateFrameBinding(PipelineRegistry& registry)
{
    if (!frame_binding)
    {
        const auto signature = registry.FrameSignature();
        if (!signature || !buffer_manager) return;

        signature->CreateShaderResourceBinding(&frame_binding, true);
        if (!frame_binding) return;

        if (const auto constants_variable = frame_binding->GetVariableByName(
            SHADER_TYPE_VERTEX, ShaderSlots::FrameConstants))
        {
            constants_variable->Set(frame_constants.RawPtr());
        }

        batch_constants_variable = frame_binding->GetVariableByName(
            SHADER_TYPE_VERTEX, ShaderSlots::BatchConstants);
    }

    if (instance_view && instance_buffer.GetVersion() != instance_version)
    {
        if (const auto variable = frame_binding->GetVariableByName(
            SHADER_TYPE_VERTEX, ShaderSlots::Instances))
        {
            variable->Set(instance_view.RawPtr());
        }

        instance_version = instance_buffer.GetVersion();
    }

    if (transform_table != InvalidPersistentTableID &&
        buffer_manager->Version(transform_table) != transform_version)
    {
        if (const auto variable = frame_binding->GetVariableByName(
            SHADER_TYPE_VERTEX, ShaderSlots::Transforms))
        {
            variable->Set(buffer_manager->View(transform_table));
        }

        transform_version = buffer_manager->Version(transform_table);
    }

    if (light_table != InvalidPersistentTableID &&
        buffer_manager->Version(light_table) != light_version)
    {
        if (const auto variable = frame_binding->GetVariableByName(
            SHADER_TYPE_PIXEL, ShaderSlots::Lights))
        {
            variable->Set(buffer_manager->View(light_table));
        }

        light_version = buffer_manager->Version(light_table);
    }
}

void RenderRuntime::CommitBatch(IDeviceContext* context, std::uint32_t instance_base) const
{
    if (!context || !frame_binding) return;

    batch_constants_variable->SetInlineConstants(&instance_base, 0, 1);
    context->CommitShaderResources(frame_binding, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}
