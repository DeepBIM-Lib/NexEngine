//
// Created by SherSpidey on 2026/7/1.
//

#include <NexEngine/Core/MathUtils.h>
#include <NexEngine/Core/Transform.h>
#include <NexEngine/Core/TransformUtils.h>
#include <NexEngine/Core/Movement.h>

#include <NexEngine/Input/Controller/Controller.h>
#include <NexEngine/Input/Controller/FlyControllerSystem.h>

#include <NexEngine/Renderer/Camera/Camera.h>

#include <NexEngine/Renderer/Camera/CameraModule.h>

using namespace Nex;

namespace
{
    void RegisterCamera(flecs::world& world)
    {
        world.component<LookAt>("LookAt")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Z");

        world.component<Camera>("Camera")
             .member<float>("Near")
             .member<float>("Far")
             .member<float>("Focal")
             .add(flecs::With, world.component<LookAt>())
             .add(flecs::With, world.component<Position>())
             .add(flecs::With, world.component<Rotation>());

        world.component<Orthographic>("Orthographic")
             .member<float>("Height")
             .add(flecs::With, world.component<Camera>());

        world.component<Perspective>("Perspective")
             .member<float>("FOV")
             .add(flecs::With, world.component<Camera>());

        world.component<ClearFlag>("ClearFlag")
             .constant("None", ClearFlag::None)
             .constant("Color", ClearFlag::Color)
             .constant("Depth", ClearFlag::Depth)
             .constant("Stencil", ClearFlag::Stencil)
             .constant("All", ClearFlag::All);

        world.component<Viewport>("Viewport")
             .member<float>("X")
             .member<float>("Y")
             .member<float>("Width")
             .member<float>("Height");

        world.component<RenderView>("RenderView")
             .member<Viewport>("Viewport")
             .member<std::uint32_t>("Order")
             .member<std::uint8_t>("ClearFlag")
             .member<Vector4>("ClearColor")
             .member<float>("ClearDepth")
             .member<std::uint8_t>("ClearStencil")
             .add(flecs::Exclusive)
             .add(flecs::OnDeleteTarget, flecs::Remove);

        world.observer<LookAt, Position>("RotationFromLookAt")
             .with<Rotation>().filter()
             .event(flecs::OnSet)
             .each([](flecs::entity entity, const LookAt& look_at, const Position& position)
             {
                 Vector3 eye = position.data;
                 Vector3 target = look_at.target;
                 Vector4 rotation;
                 if (const auto pt = GetParentTransform(entity))
                 {
                     eye = TransformPoint(pt->data, position.data);
                     const auto quat = LookAtQuaternion(eye, target);
                     const auto p_quat = RotationFromMatrix(pt->data);

                     rotation = QuaternionMultiply(quat, QuaternionConjugate(p_quat));
                 }
                 else
                     rotation = LookAtQuaternion(eye, target);

                 if (auto camera = entity.try_get_mut<Camera>())
                     camera->focal = length(eye - target);

                 entity.set<Rotation>({rotation});
             });
    }

    void RegisterController(flecs::world& world)
    {
        world.component<Controller>("Controller")
             .add(flecs::With, world.component<Velocity>())
             .add(flecs::With, world.component<AngularVelocity>());

        world.system<LookAt, const Camera, const Transform>("SyncCameraLookAt")
             .kind(flecs::OnStore)
             .with<Controller>()
             .each([](LookAt& look_at, const Camera& camera, const Transform& transform)
             {
                 const auto position = TranslationFromMatrix(transform.data);
                 const auto forward = ToForward(transform.data);

                 look_at.target = position + forward * camera.focal;
             });

        FlyControllerSystem::Register(world);
    }
}

CameraModule::CameraModule(flecs::world& world)
{
    world.module<CameraModule>("NexEngine::Camera");

    RegisterCamera(world);

    RegisterController(world);
}
