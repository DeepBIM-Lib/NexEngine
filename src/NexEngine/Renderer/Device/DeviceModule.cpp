//
// Created by SherSpidey on 2026/8/5.
//

#include <NexEngine/Renderer/Device/Surface.h>
#include <NexEngine/Renderer/Device/SurfaceResource.h>
#include <NexEngine/Renderer/Device/GraphicsAdapterDetails.h>
#include <NexEngine/Renderer/Render/RenderCore.h>

#include <NexEngine/App/EngineContext.h>
#include <NexEngine/Utils/ResourceManager.h>

#include <NexEngine/Renderer/Device/DeviceModule.h>

using namespace Nex;

namespace
{
    template <typename T>
    SurfaceHandle CreateSurface(ResourceManager& resource_manager,
                                const RenderCore& core,
                                const Surface& surface,
                                const T& data)
    {
        return resource_manager.CreateResource<SurfaceResource>(core, surface, data);
    }

    void RegisterDevice(flecs::world& world)
    {
        world.component<GraphicsAdapterDetails>("GraphicsAdapterDetails")
             .add(flecs::Singleton);
    }

    void RegisterSurface(flecs::world& world)
    {
        world.component<NativeWindowType>("NativeWindowType")
             .constant("Win32", NativeWindowType::Win32)
             .constant("X11", NativeWindowType::X11)
             .constant("Wayland", NativeWindowType::Wayland);

        world.component<ColorFormat>("ColorFormat")
             .constant("RGBA8_UNORM_SRGB", ColorFormat::RGBA8_UNORM_SRGB)
             .constant("RGBA8_UNORM", ColorFormat::RGBA8_UNORM)
             .constant("BGRA8_UNORM_SRGB", ColorFormat::BGRA8_UNORM_SRGB)
             .constant("BGRA8_UNORM", ColorFormat::BGRA8_UNORM)
             .constant("RGBA16_FLOAT", ColorFormat::RGBA16_FLOAT);

        world.component<WindowSurface>("WindowSurface")
             .member<String>("Title")
             .member<NativeWindowType>("Type")
             .member<std::uint64_t>("ID");

        world.component<OffscreenSurface>("OffscreenSurface")
             .member<String>("Name");

        world.component<SurfaceSource>("SurfaceSource")
             .add(flecs::Exclusive);

        world.component<SurfaceHandle>("SurfaceHandle")
             .member<std::uint32_t>("Index")
             .member<std::uint32_t>("Generation");

        world.component<Surface>("Surface")
             .member<std::uint32_t>("Width")
             .member<std::uint32_t>("Height")
             .member<ColorFormat>("ColorFormat")
             .add(flecs::With, world.component<SurfaceHandle>());

        world.observer<SurfaceSource>("OnSetSurfaceSource")
             .term_at(0).second(flecs::Wildcard)
             .event(flecs::OnSet)
             .each([](flecs::entity entity, SurfaceSource)
             {
                 if (const auto surface = entity.try_get<Surface>())
                 {
                     auto world = entity.world();
                     auto& resource_manager = world.get_mut<ResourceManager>();
                     auto& engine_context = world.get_mut<EngineContext>();
                     const auto& render_core = world.get<RenderCore>();

                     auto& handle = entity.get_mut<SurfaceHandle>();

                     // try to release old handle if exits
                     engine_context.retirement_signal_requested |= resource_manager.DestroyResource(
                         handle, engine_context.render_frame_index + 1
                     );

                     auto target = entity.target<SurfaceSource>();
                     if (target == world.component<WindowSurface>())
                     {
                         auto source = static_cast<const WindowSurface*>(entity.try_get(
                             world.component<SurfaceSource>(), target.id()));

                         handle = CreateSurface(resource_manager, render_core, *surface, *source);
                     }
                     else if (target == world.component<OffscreenSurface>())
                     {
                         auto source = static_cast<const OffscreenSurface*>(entity.try_get(
                             world.component<SurfaceSource>(), target.id()));

                         handle = CreateSurface(resource_manager, render_core, *surface, *source);
                     }
                 }
             });

        world.observer<Surface>("OnSetSurface")
             .event(flecs::OnSet)
             .each([](flecs::entity entity, Surface& surface)
             {
                 auto& handle = entity.get_mut<SurfaceHandle>();

                 auto world = entity.world();
                 auto& resource_manager = world.get_mut<ResourceManager>();
                 const auto& render_core = world.get<RenderCore>();

                 // valid surface resource
                 if (auto surface_resource = resource_manager.GetResource(handle))
                 {
                     surface_resource->Resize(surface.width, surface.height);
                     return;
                 }

                 // create surface resource
                 if (const auto window_surface = entity.try_get<SurfaceSource, WindowSurface>())
                 {
                     handle = CreateSurface(resource_manager, render_core, surface, *window_surface);
                 }
                 else if (const auto offscreen = entity.try_get<SurfaceSource, OffscreenSurface>())
                 {
                     handle = CreateSurface(resource_manager, render_core, surface, *offscreen);
                 }
             });

        world.observer<Surface>("OnRemoveSurface")
             .event(flecs::OnRemove)
             .each([](flecs::entity entity, Surface&)
             {
                 auto& handle = entity.get_mut<SurfaceHandle>();

                 auto world = entity.world();
                 auto& engine_context = world.get_mut<EngineContext>();
                 auto& resource_manager = world.get_mut<ResourceManager>();

                 engine_context.retirement_signal_requested |= resource_manager.DestroyResource(
                     handle, engine_context.render_frame_index + 1
                 );
             });
    }
}

DeviceModule::DeviceModule(flecs::world& world)
{
    world.module<DeviceModule>("NexEngine::Device");

    RegisterDevice(world);

    RegisterSurface(world);
}
