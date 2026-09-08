//
// Created by SherSpidey on 2026/6/24.
//

#include <NexEngine/App/EngineModule.h>

#include <NexEngine/App/EngineContext.h>

#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/MeshManager.h>
#include <NexEngine/Utils/MaterialManager.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

#include <NexEngine/Core/CoreModule.h>
#include <NexEngine/Input/InputModule.h>
#include <NexEngine/Renderer/Camera/CameraModule.h>
#include <NexEngine/Geometry/GeometryModule.h>
#include <NexEngine/Renderer/Device/DeviceModule.h>
#include <NexEngine/Renderer/Lighting/LightingModule.h>
#include <NexEngine/Renderer/Material/MaterialModule.h>
#include <NexEngine/Renderer/RendererModule.h>

using namespace Nex;

namespace
{
    void RegisterMathScript(flecs::world& world)
    {
        ECS_IMPORT(world, FlecsScriptMath);
    }

    void RegisterWorldComponents(flecs::world& world)
    {
        world.component<EngineContext>("EngineContext")
             .add(flecs::Singleton);

        world.component<PersistentBufferManager>("PersistentBufferManager")
             .add(flecs::Singleton);

        world.component<ResourceManager>("ResourceManager")
             .add(flecs::Singleton);

        world.component<MeshManager>("MeshManager")
             .add(flecs::Singleton);

        world.component<MaterialManager>("MaterialManager")
             .add(flecs::Singleton);
    }
}

EngineModule::EngineModule(flecs::world& world)
{
    RegisterMathScript(world);

    world.module<EngineModule>("::NexEngine");

    RegisterWorldComponents(world);

    world.import<CoreModule>();
    world.import<InputModule>();
    world.import<DeviceModule>();
    world.import<CameraModule>();
    world.import<GeometryModule>();
    world.import<MaterialModule>();
    world.import<LightingModule>();
    world.import<RendererModule>();
}
