//
// Created by SherSpidey on 2026/7/9.
//

#include <NexEngine/Core/Transform.h>

#include <NexEngine/Geometry/Mesh.h>
#include <NexEngine/Geometry/Primitives.h>

#include <NexEngine/Utils/MeshManager.h>
#include <NexEngine/Utils/ResourceManager.h>

#include <NexEngine/Geometry/GeometryModule.h>

using namespace Nex;

namespace
{
    void SyncPrimitiveScale(flecs::entity entity, PrimitiveType type, const void* data)
    {
        if (!IsAffine(type)) return;

        PrimitiveScale scale{Vector3One()};

        switch (type)
        {
        case PrimitiveType::Cube:
            {
                auto cube = static_cast<const Cube*>(data);

                scale.scale.x = cube->x;
                scale.scale.y = cube->y;
                scale.scale.z = cube->z;
                break;
            }
        case PrimitiveType::QuadSphere:
            {
                auto sphere = static_cast<const QuadSphere*>(data);

                scale.scale.x = sphere->radius;
                scale.scale.y = sphere->radius;
                scale.scale.z = sphere->radius;
                break;
            }
        default:
            break;
        }

        entity.set<PrimitiveScale>(scale);
    }

    void RegisterPrimitives(flecs::world& world)
    {
        world.component<PrimitiveType>("PrimitiveType")
             .constant("Cube", PrimitiveType::Cube)
             .constant("QuadSphere", PrimitiveType::QuadSphere);

        world.component<Cube>("Cube")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .set<PrimitiveType>(PrimitiveType::Cube);

        world.component<Sphere>("Sphere")
             .member<float>("Radius")
             .set<PrimitiveType>(PrimitiveType::QuadSphere);

        world.component<QuadSphere>("QuadSphere")
             .member<float>("Radius")
             .set<PrimitiveType>(PrimitiveType::QuadSphere);

        world.component<MeshHandle>("MeshHandle")
             .on_replace([](flecs::entity entity, MeshHandle& previous, MeshHandle&)
             {
                 ResourceManager::TryRelease<MeshManager>(entity, previous);
             })
             .on_remove([](flecs::entity entity, MeshHandle& handle)
             {
                 ResourceManager::TryRelease<MeshManager>(entity, handle);
             })
             .member<std::uint32_t>("Index")
             .member<std::uint32_t>("Generation")
             .add(flecs::With, world.component<Transform>());

        world.component<PrimitiveSource>("PrimitiveSource")
             .add(flecs::Exclusive);

        world.component<PrimitiveScale>("PrimitiveScale")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z");

        world.observer<PrimitiveSource>("OnSetPrimitives")
             .term_at(0).second(flecs::Wildcard)
             .event(flecs::OnSet)
             .each([](flecs::entity entity, PrimitiveSource)
             {
                 auto target = entity.target<PrimitiveSource>();
                 const auto& type = target.get<PrimitiveType>();

                 auto world = entity.world();
                 auto& mesh_manager = world.get_mut<MeshManager>();
                 auto& resource_manager = world.get_mut<ResourceManager>();

                 const auto data = entity.try_get(
                     world.component<PrimitiveSource>(), target.id());

                 // sync scale
                 SyncPrimitiveScale(entity, type, data);

                 auto handle = mesh_manager.CreatePrimitive(type, data, resource_manager);

                 if (IsHandleValid(handle)) entity.set<MeshHandle>(handle);
             });

        world.observer<PrimitiveSource>("OnRemovePrimitives")
             .term_at(0).second(flecs::Wildcard)
             .event(flecs::OnRemove)
             .each([](flecs::iter& it, std::size_t index, PrimitiveSource)
             {
                 if (auto other = it.other_table();
                     other.get_table() && other.has<PrimitiveSource>(flecs::Wildcard))
                     return;

                 it.entity(index).remove<MeshHandle>();
             });
    }
}

GeometryModule::GeometryModule(flecs::world& world)
{
    world.module<GeometryModule>("NexEngine::Geometry");

    RegisterPrimitives(world);
}
