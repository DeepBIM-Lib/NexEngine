//
// Created by SherSpidey on 2026/8/25.
//

#include <NexEngine/Core/MathUtils.h>
#include <NexEngine/Core/Transform.h>

#include <NexEngine/Geometry/Mesh.h>
#include <NexEngine/Geometry/Primitives.h>

#include <NexEngine/Renderer/Renderer.h>
#include <NexEngine/Renderer/Render/RenderObject.h>
#include <NexEngine/Renderer/Render/RenderUtils.h>

#include <NexEngine/Utils/PersistentBufferManager.h>

#include <NexEngine/Renderer/Render/RenderPipelineModule.h>

using namespace Nex;

namespace
{
    void RegisterTransformRenderer(flecs::world& world)
    {
        world.component<TransformID>("TransformID")
             .member<std::uint32_t>("ID");

        world.observer<const Renderer,
                       const MeshHandle,
                       const Transform,
                       const PrimitiveScale*>("UploadTransform")
             .without<TransformID>().filter()
             .event(flecs::OnAdd)
             .each([](flecs::entity entity, const Renderer&, const MeshHandle&,
                      const Transform& transform, const PrimitiveScale* scale)
             {
                 auto world = entity.world();
                 auto& render_runtime = world.get_mut<RenderRuntime>();

                 const TransformID transform_id{AllocateTransform(render_runtime)};
                 UpdateTransform(render_runtime, transform, transform_id, scale);

                 entity.set<TransformID>(transform_id);
             });

        world.observer<const Renderer,
                       const MeshHandle,
                       const Transform>("UnloadTransform")
             .with<TransformID>().filter()
             .event(flecs::OnRemove)
             .each([](flecs::entity entity, const Renderer&, const MeshHandle&, const Transform&)
             {
                 auto world = entity.world();
                 auto& render_runtime = world.get_mut<RenderRuntime>();

                 const auto& [id] = entity.get<TransformID>();
                 ReleaseTransform(render_runtime, id);

                 entity.remove<TransformID>();
             });

        world.observer<PrimitiveScale>("OnStaticPrimitiveScaleUpdate")
             .without<Kinematic>()
             .without<Rigidbody>()
             .with<TransformID>().filter()
             .event(flecs::OnSet)
             .each([](flecs::entity entity, PrimitiveScale&)
             {
                 auto world = entity.world();
                 auto& render_runtime = world.get_mut<RenderRuntime>();

                 UpdateTransform(render_runtime, entity);
             });

        // static transform update
        world.observer<Transform>("OnStaticTransformUpdate")
             .without<Kinematic>()
             .without<Rigidbody>()
             .with<TransformID>().filter()
             .event(flecs::OnSet)
             .each([](flecs::entity entity, Transform&)
             {
                 auto world = entity.world();
                 auto& render_runtime = world.get_mut<RenderRuntime>();

                 UpdateTransform(render_runtime, entity);
             });

        // dynamic transform update
        world.system<const Transform, const TransformID, const PrimitiveScale*>("DynamicTransformUpdate")
             .with<Kinematic>()
             .without<Rigidbody>()
             .kind(flecs::PreStore)
             .run([](flecs::iter& it)
             {
                 auto& render_runtime = it.world().get_mut<RenderRuntime>();

                 while (it.next())
                 {
                     auto tf_field = it.field<const Transform>(0);
                     auto id_field = it.field<const TransformID>(1);
                     auto scale_field = it.field<const PrimitiveScale>(2);
                     bool has_scale = it.is_set(2);

                     for (auto i : it)
                     {
                         auto scale = has_scale ? &scale_field[i] : nullptr;
                         UpdateTransform(render_runtime, tf_field[i], id_field[i], scale);
                     }
                 }
             });
    }
}

void RenderPipelineModule::Register(flecs::world& world)
{
    RegisterTransformRenderer(world);
}
