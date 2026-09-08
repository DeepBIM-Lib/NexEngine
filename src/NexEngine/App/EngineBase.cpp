//
// Created by SherSpidey on 2026/5/26.
//

#include <NexEngine/Core/EntityUtils.h>

#include <NexEngine/App/EngineModule.h>
#include <NexEngine/App/EngineContext.h>

#include <NexEngine/Renderer/Camera/Camera.h>
#include <NexEngine/Renderer/Material/MaterialBindingCache.h>
#include <NexEngine/Renderer/Render/RenderCore.h>
#include <NexEngine/Renderer/Render/RenderFrame.h>
#include <NexEngine/Renderer/Render/RenderRuntime.h>
#include <NexEngine/Renderer/Device/GraphicsAdapterDetails.h>
#include <NexEngine/Renderer/Pipeline/PipelineRegistry.h>

#include <NexEngine/Utils/MeshManager.h>
#include <NexEngine/Utils/MaterialManager.h>
#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

#include <NexEngine/App/EngineBase.h>

using namespace Nex;
using namespace Diligent;

void EngineBase::ImportECSModules()
{
    world.import<flecs::stats>();

    world.import<EngineModule>();
}

void EngineBase::CreateDeviceAndResource(Render_Device_Type type, const String& shader_path)
{
    // engine singleton component init
    world.emplace<GraphicsAdapterDetails>(GraphicsAdapterDetails{});
    auto& graphic_details = world.get_mut<GraphicsAdapterDetails>();

    world.emplace<RenderCore>(RendererCoreDesc{.type = type, .shader_path = shader_path.c_str()}, graphic_details);
    const auto& render_core = world.get<RenderCore>();

    world.emplace<PersistentBufferManager>(render_core.device);
    auto buffer_manager = world.try_get_mut<PersistentBufferManager>();
    
    world.emplace<MaterialBindingCache>();

    world.emplace<RenderFrame>(world);

    world.emplace<RenderRuntime>(render_core.device, buffer_manager);

    world.emplace<EngineContext>(render_core.device);

    world.emplace<PipelineRegistry>(render_core.device, render_core.shader_factory);

    world.emplace<ResourceManager>();
    auto& resource_manager = world.get_mut<ResourceManager>();

    world.emplace<MeshManager>(render_core.device, render_core.context, resource_manager);

    world.emplace<MaterialManager>(render_core.device, buffer_manager, resource_manager);
}

void EngineBase::InitECSWorld(const String& script) const
{
    if (!script.empty())
        world.script()
             .filename(script.c_str())
             .run();

    // default off
    world.set<flecs::Rest>({});
}

void EngineBase::Destroy()
{
    if (const auto* render_core = world.try_get<RenderCore>())
        render_core->context->WaitForIdle();

    delete this;
}

Entity EngineBase::CreateSurface(const SurfaceDesc& desc)
{
    if (const auto window = desc.data.get<WindowSurface>())
    {
        auto entity = world.entity(window->title.c_str_prefix("Surface: "));

        entity.set<SurfaceSource, WindowSurface>(*window);
        entity.set<Surface>(desc.surface);

        // bind to main camera
        auto camera = world.lookup("MainScene::MainCamera");
        camera.set<RenderView>(entity, {});

        return EntityCast(entity);
    }

    if (const auto offscreen = desc.data.get<OffscreenSurface>())
    {
        auto entity = world.entity(offscreen->name.c_str());

        return EntityCast(entity);
    }

    return {};
}

void EngineBase::CleanSurface(Entity entity)
{
    auto surface_entity = EntityCast(world, entity);
    surface_entity.destruct();
}

void EngineBase::ResizeSurface(Entity entity, std::uint32_t width, std::uint32_t height)
{
    auto surface_entity = EntityCast(world, entity);
    auto& surface = surface_entity.get_mut<Surface>();

    surface.width = width;
    surface.height = height;

    surface_entity.modified<Surface>();
}

void EngineBase::PushInput(const InputEvent& event)
{
    auto& input_buffer = world.get_mut<InputBuffer>();
    input_buffer.events.emplace_back(event);
}

void EngineBase::SubmitCommand(const Command&)
{
}

void EngineBase::PollResponse(ResponseCallback, void*)
{
}

void EngineBase::Tick()
{
    auto& engine_context = world.get_mut<EngineContext>();
    auto& resource_manager = world.get_mut<ResourceManager>();

    ++engine_context.submit_serial;
    resource_manager.Collect(engine_context.render_fence->GetCompletedValue());
}

void EngineBase::SubmitRetiredSignal() const
{
    auto& engine_context = world.get_mut<EngineContext>();
    if (!engine_context.retirement_signal_requested) return;

    const auto& render_core = world.get<RenderCore>();

    render_core.context->EnqueueSignal(
        engine_context.render_fence, ++engine_context.render_frame_index
    );

    engine_context.retirement_signal_requested = false;
}

void EngineBase::FinishFrame() const
{
    const auto& render_core = world.get<RenderCore>();

    render_core.context->Flush();
    render_core.context->FinishFrame();

    render_core.device->ReleaseStaleResources();
}
