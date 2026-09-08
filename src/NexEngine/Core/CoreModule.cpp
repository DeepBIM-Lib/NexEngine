//
// Created by SherSpidey on 2026/6/24.
//

#include <NexEngine/Core/Scene.h>

#include <NexEngine/Core/MathUtils.h>
#include <NexEngine/Core/Movement.h>
#include <NexEngine/Core/Transform.h>
#include <NexEngine/Core/TransformUtils.h>

#include <NexEngine/Core/MetaSerializer.h>

#include <NexEngine/Core/CoreModule.h>

using namespace Nex;

namespace
{
    void PropagateTransform(flecs::entity entity)
    {
        const auto pt = entity.try_get<Transform>();

        entity.children([&](flecs::entity child)
        {
            if (UpdateTransform(child, pt))
            {
                // notify to renderer
                child.modified<Transform>();

                PropagateTransform(child);
            }
        });
    }

    template <typename Component>
    void RegisterTransformComponentObserver(flecs::world& world, const char* name)
    {
        world.observer<Component>(name)
             .template without<Kinematic>()
             .template without<Rigidbody>()
             .event(flecs::OnSet)
             .each([](flecs::entity entity, Component&)
             {
                 const auto pt = GetParentTransform(entity);
                 UpdateTransform(entity, pt);

                 // notify to renderer
                 entity.modified<Transform>();

                 PropagateTransform(entity);
             });
    }

    void RegisterMath(flecs::world& world)
    {
        world.component<Vector2>("Vec2")
             .member<float>("X")
             .member<float>("Y");

        world.component<Vector3>("Vec3")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z");

        world.component<Vector4>("Vec4")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .member<float>("W");

        world.component<Color>("Color")
             .member<std::uint8_t>("R")
             .member<std::uint8_t>("G")
             .member<std::uint8_t>("B")
             .member<std::uint8_t>("A");

        world.component<Matrix4>("Mat4")
             .member<Vector4>("R0")
             .member<Vector4>("R1")
             .member<Vector4>("R2")
             .member<Vector4>("R3");
    }

    void RegisterTransform(flecs::world& world)
    {
        world.component<Transform>("Transform")
             .member<Vector4>("R0")
             .member<Vector4>("R1")
             .member<Vector4>("R2")
             .member<Vector4>("R3");

        world.component<Position>("Position")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .add(flecs::With, world.component<Transform>());

        world.component<Rotation>("Rotation")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .member<float>("W")
             .add(flecs::With, world.component<Position>());

        world.component<Scale>("Scale")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .add(flecs::With, world.component<Position>());

        world.component<Rotor>("Rotor")
             .member<Vector4>("BaseRotation")
             .member<float>("Pitch")
             .member<float>("Yaw")
             .member<float>("Roll")
             .add(flecs::With, world.component<Rotation>());

        world.component<Kinematic>("Kinematic");
        world.component<Rigidbody>("Rigidbody");

        // static transform stuff
        RegisterTransformComponentObserver<Position>(world, "OnSetPosition");
        RegisterTransformComponentObserver<Rotation>(world, "OnSetRotation");
        RegisterTransformComponentObserver<Scale>(world, "OnSetScale");

        // kinematic transform stuff
        world.observer<>("KinematicPropagate")
             .with<Kinematic>().up()
             .without<Kinematic>()
             .event(flecs::OnAdd)
             .each([](flecs::entity entity)
             {
                 entity.add<Kinematic>();
             });

        // rotor on add
        world.observer<Rotor>("RotorSetup")
             .with<Rotation>().filter()
             .event(flecs::OnAdd)
             .each([](flecs::entity entity, Rotor& look_at)
             {
                 const auto& rotation = entity.get<Rotation>();
                 look_at.base = rotation.data;
             });

        // compose rotation from rotor
        world.system<Rotation, const Rotor>("RotorUpdate")
             .with<Kinematic>()
             .without<Rigidbody>()
             .kind(flecs::PreStore)
             .each([](Rotation& rotation, const Rotor& rotor)
             {
                 rotation.data = ComposeRotorRotation(rotor);
             });

        // using ChildOf flag
        auto child_of_query =
            world.query_builder<
                     Transform,
                     const Position,
                     const Rotation*,
                     const Scale*,
                     const Transform*>("KinematicTransformChildOf")
                 .term_at(4).cascade()
                 .without<flecs::Parent>()
                 .with<Kinematic>()
                 .without<Rigidbody>()
                 .cache_kind(flecs::QueryCacheAuto)
                 .build();

        // using Parent flg
        auto parent_query =
            world.query_builder<
                     Transform,
                     const Position,
                     const Rotation*,
                     const Scale*,
                     const flecs::Parent>("KinematicTransformParent")
                 .with<Kinematic>()
                 .without<Rigidbody>()
                 .group_by(flecs::ParentDepth)
                 .cache_kind(flecs::QueryCacheAuto)
                 .build();

        world.system<>("KinematicTransformUpdate")
             .kind(flecs::PreStore)
             .run([child_of_query, parent_query](flecs::iter&)
             {
                 // update transform without "Parent"
                 child_of_query.each([](Transform& t, const Position& p,
                                        const Rotation* r, const Scale* s, const Transform* pt)
                 {
                     t = TransformCompose(p, r, s, pt);
                 });

                 // update transform with "Parent"
                 for (auto& [depth, _] : parent_query.groups())
                 {
                     parent_query.set_group(depth).each(
                         [](flecs::iter& it, std::size_t i, Transform& t, const Position& p,
                            const Rotation* r, const Scale* s, const flecs::Parent&)
                         {
                             auto entity = it.entity(i);
                             const auto pt = GetParentTransform(entity);
                             t = TransformCompose(p, r, s, pt);
                         });
                 }
             });
    }

    void RegisterMovement(flecs::world& world)
    {
        world.component<Velocity>("Velocity")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .add(flecs::With, world.component<Kinematic>())
             .add(flecs::With, world.component<Position>());

        world.component<AngularVelocity>("AngularVelocity")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .add(flecs::With, world.component<Kinematic>())
             .add(flecs::With, world.component<Rotor>());

        world.component<Acceleration>("Acceleration")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .add(flecs::With, world.component<Velocity>());

        world.component<AngularAcceleration>("AngularAcceleration")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z")
             .add(flecs::With, world.component<AngularVelocity>());

        world.system<Position, const Velocity>("Move")
             .kind(flecs::PostUpdate)
             .each([](flecs::iter& it,
                      std::size_t, Position& position, const Velocity& velocity)
             {
                 const auto delta_time = it.delta_time();
                 position.data += velocity.data * delta_time;
             });

        world.system<Rotor, const AngularVelocity>("Rotate")
             .kind(flecs::PostUpdate)
             .each([](flecs::iter& it,
                      std::size_t, Rotor& rotor, const AngularVelocity& velocity)
             {
                 const auto delta_time = it.delta_time();

                 rotor.data += velocity.data * delta_time;
             });

        world.system<Velocity, const Acceleration>("Accelerate")
             .kind(flecs::PostUpdate)
             .each([](flecs::iter& it,
                      std::size_t, Velocity& velocity, const Acceleration& acceleration)
             {
                 const float delta_time = it.delta_time();
                 velocity.data += acceleration.data * delta_time;
             });

        world.system<AngularVelocity, const AngularAcceleration>("AngularAccelerate")
             .kind(flecs::PostUpdate)
             .each([](flecs::iter& it,
                      std::size_t, AngularVelocity& velocity, const AngularAcceleration& acceleration)
             {
                 const float delta_time = it.delta_time();
                 velocity.data += acceleration.data * delta_time;
             });
    }

    void RegisterScene(flecs::world& world)
    {
        world.component<Scene>("Scene")
             .member<std::uint32_t>("Index");
    }
}

CoreModule::CoreModule(flecs::world& world)
{
    world.module<CoreModule>("NexEngine::Core");

    RegisterMath(world);

    RegisterMetaString(world);
    RegisterMetaArray<Vector2>(world);
    RegisterMetaArray<Vector3>(world);
    RegisterMetaArray<Vector4>(world);

    RegisterMetaArray<Color>(world);

    RegisterMetaArray<std::uint32_t>(world);

    RegisterTransform(world);
    RegisterMovement(world);

    RegisterScene(world);
}
