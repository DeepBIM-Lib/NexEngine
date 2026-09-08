//
// Created by SherSpidey on 2026/8/25.
//

#include <limits>
#include <algorithm>

#include <NexEngine/Core/Scene.h>
#include <NexEngine/Core/MathUtils.h>

#include <NexEngine/Geometry/MeshResource.h>

#include <NexEngine/Renderer/Lighting/Light.h>

#include <NexEngine/Renderer/Device/Surface.h>
#include <NexEngine/Renderer/Material/MaterialResource.h>

#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/MaterialManager.h>

#include <NexEngine/Renderer/Render/RenderFrame.h>

using namespace Nex;

namespace
{
    // pack 64-bit sort key: [scene: 4b | type: 4b | descriptor: 24b | mesh: 20b | submesh: 12b]
    [[nodiscard]] std::uint64_t MakeSortKey(std::uint32_t scene_index,
                                            MaterialType type,
                                            DescriptorHandle descriptor,
                                            MeshHandle mesh,
                                            std::uint32_t submesh_index) noexcept
    {
        return ((scene_index & 0xFULL) << 60) |
            ((raw(type) & 0xFULL) << 56) |
            ((HandleKey(descriptor) & 0xFFFFFFULL) << 32) |
            ((HandleKey(mesh) & 0xFFFFFULL) << 12) |
            (static_cast<std::uint64_t>(submesh_index) & 0xFFFULL);
    }

    // 8-pass lsd radix sort with intro-sort fallback for small buckets
    void RadixSort64(std::vector<SortItem>& items, std::vector<SortItem>& temp)
    {
        const auto count = items.size();
        if (count <= 1) return;

        // fallback to intro-sort for tiny item counts
        if (count <= 64)
        {
            std::ranges::sort(items, {}, &SortItem::key);
            return;
        }

        temp.resize(count);

        auto* src = items.data();
        auto* dst = temp.data();

        for (std::uint32_t pass = 0; pass < 8; ++pass)
        {
            const std::uint32_t shift = pass * 8;
            std::array<std::size_t, 256> counts{};

            // histogram counts
            for (std::size_t i = 0; i < count; ++i)
            {
                const auto radix = static_cast<std::uint8_t>((src[i].key >> shift) & 0xFF);
                ++counts[radix];
            }

            // prefix sum offsets
            std::array<std::size_t, 256> offsets{};
            std::size_t accumulated = 0;
            for (std::size_t i = 0; i < 256; ++i)
            {
                offsets[i] = accumulated;
                accumulated += counts[i];
            }

            // scatter to destination
            for (std::size_t i = 0; i < count; ++i)
            {
                const auto radix = static_cast<std::uint8_t>((src[i].key >> shift) & 0xFF);
                dst[offsets[radix]++] = src[i];
            }

            // swap ping-pong buffers
            std::swap(src, dst);
        }
    }
}

RenderFrame::RenderFrame(flecs::world& world)
{
    auto prev_scope = world.get_scope();

    auto renderer_scope = world.entity("NexEngine::Renderer");
    world.set_scope(renderer_scope);
    
    scene_query_ = world.query_builder<const Scene>("ExtractSceneLighting")
                        .cache_kind(flecs::QueryCacheAuto)
                        .build();

    camera_query_ = world.query_builder<const Camera,
                                        const Transform,
                                        const RenderView,
                                        const Scene*>("ExtractCamera")
                         .term_at(2).second(flecs::Wildcard)
                         .term_at(3).up(flecs::ChildOf)
                         .cache_kind(flecs::QueryCacheAuto)
                         .build();

    instance_query_ = world.query_builder<const MeshHandle,
                                          const TransformID,
                                          const Renderer,
                                          const MaterialHandle*,
                                          const Scene>("ExtractInstance")
                           .term_at(4).up(flecs::ChildOf)
                           .cache_kind(flecs::QueryCacheAuto)
                           .build();

    world.set_scope(prev_scope);
}

void RenderFrame::Extract(flecs::world& world)
{
    ExtractSceneLighting();
    ExtractRenderViews();
    ExtractInstances(world);
}

void RenderFrame::ExtractSceneLighting()
{
    // reset
    for (auto& scene_meta : scenes_)
    {
        scene_meta.lighting = {};
    }

    scene_query_.each([&](flecs::entity entity, const Scene& scene)
    {
        if (scene.index >= MaxActiveScenes) return;

        auto& lighting = scenes_[scene.index].lighting;

        // extract ambient_color / environment lighting
        if (const auto env = entity.try_get<EnvironmentLight>())
        {
            lighting.environment.ambient_color = env->ambient_color;
            lighting.environment.ambient_intensity = env->ambient_intensity;
        }

        // extract sun / directional light
        if (const auto sun = entity.try_get<DirectionalLight>())
        {
            lighting.sun.direction = sun->direction;
            lighting.sun.intensity = sun->intensity;
            lighting.sun.color = sun->color;
        }
    });
}

void RenderFrame::ExtractRenderViews()
{
    render_views_.clear();

    camera_query_.each([&](flecs::entity entity,
                           const Camera& camera,
                           const Transform& transform,
                           const RenderView& render_view,
                           const Scene* scene)
    {
        auto surface_entity = entity.target<RenderView>();
        if (!surface_entity.is_alive()) return;

        const auto surface = surface_entity.try_get<Surface>();
        const auto surface_handle = surface_entity.try_get<SurfaceHandle>();
        if (!surface || !surface_handle || surface->width == 0 || surface->height == 0) return;

        const auto surf_w = static_cast<float>(surface->width);
        const auto surf_h = static_cast<float>(surface->height);

        const auto vp_width = surf_w * render_view.viewport.width;
        const auto vp_height = surf_h * render_view.viewport.height;
        const auto aspect_ratio = vp_height > 0.0f ? vp_width / vp_height : 1.0f;

        RenderViewItem item;

        item.surface = *surface_handle;
        item.order = render_view.order;

        item.viewport.x = surf_w * render_view.viewport.x;
        item.viewport.y = surf_h * render_view.viewport.y;
        item.viewport.width = vp_width;
        item.viewport.height = vp_height;

        if (const auto view_mask = surface_entity.try_get<RenderTopoFlag>())
        {
            item.view_mask = *view_mask;
        }

        item.clear_flags = render_view.clear_flags;
        item.clear_color = render_view.clear_color;
        item.clear_depth = render_view.clear_depth;
        item.clear_stencil = render_view.clear_stencil;

        item.view_matrix = RigidInverse(transform.data);

        // get the right projection matrix
        if (const auto perspective = entity.try_get<Perspective>())
        {
            item.projection_matrix = PerspectiveMatrix(ToRadians(perspective->fov),
                                                       aspect_ratio, camera.near, camera.far);
        }
        else if (const auto orthographic = entity.try_get<Orthographic>())
        {
            const auto ortho_height = orthographic->height;
            const auto ortho_width = ortho_height * aspect_ratio;

            item.projection_matrix = OrthographicMatrix(ortho_width, ortho_height,
                                                        camera.near, camera.far);
        }

        // extract global position
        item.camera_position = TranslationFromMatrix(transform.data);

        // resolve bound scene slot
        item.scene_index = scene && scene->index < MaxActiveScenes ? scene->index : 0;

        render_views_.emplace_back(item);
    });

    std::ranges::sort(render_views_, [](const RenderViewItem& lhs, const RenderViewItem& rhs)
    {
        auto lhs_surface_key = HandleKey(lhs.surface);
        auto rhs_surface_key = HandleKey(rhs.surface);
        if (lhs_surface_key != rhs_surface_key)
            return lhs_surface_key < rhs_surface_key;

        return lhs.order < rhs.order;
    });
}

void RenderFrame::ExtractInstances(flecs::world& world)
{
    // reset
    instances_.clear();
    batches_.clear();
    for (auto& scene_meta : scenes_)
    {
        scene_meta.topology_ranges.fill({0, 0});
    }
    for (auto& bucket : sort_items_) bucket.clear();
    for (auto& bucket : payload_items_) bucket.clear();

    auto& resource_manager = world.get_mut<ResourceManager>();
    const auto& material_manager = world.get<MaterialManager>();
    const auto fallback_material = material_manager.GetFallback();

    // gather and bucket instances
    instance_query_.each([&](flecs::entity entity,
                             const MeshHandle& mesh_handle,
                             const TransformID& transform_id,
                             const Renderer& renderer,
                             const MaterialHandle* material_handle,
                             const Scene& scene)
    {
        if (transform_id.id == InvalidPersistentSlotIndex) return;
        if (!renderer.visible) return;
        if (scene.index >= MaxActiveScenes) return;

        // resolve resources with material fallback
        const auto actual_material = material_handle ? *material_handle : fallback_material;
        const auto mesh_resource = resource_manager.GetResource(mesh_handle);
        const auto material_resource = resource_manager.GetResource(actual_material);

        if (!mesh_resource || !material_resource) return;

        // resolve pipeline material type
        auto material_type = MaterialType::None;
        if (material_resource->data.model.get<PBRMaterial>())
            material_type = MaterialType::PBR;
        else if (material_resource->data.model.get<PhongMaterial>())
            material_type = MaterialType::Phong;
        if (material_type == MaterialType::None) return;

        // bucket submeshes by topology
        const auto mesh_num = mesh_resource->data.submeshes.size();
        for (std::uint32_t sub_idx = 0; sub_idx < mesh_num; ++sub_idx)
        {
            const auto& submesh = mesh_resource->data.submeshes[sub_idx];
            if (submesh.index_count == 0) continue;

            const auto sub_flag = ToRenderTopoFlag(submesh.topology);
            if (!HasAnyFlag(renderer.render_type, sub_flag)) continue;

            const auto topo_idx = raw(submesh.topology);
            if (topo_idx >= sort_items_.size()) continue;

            const auto payload_idx = static_cast<std::uint32_t>(payload_items_[topo_idx].size());
            payload_items_[topo_idx].push_back(InstanceItem{
                .type = material_type,
                .descriptor = material_resource->descriptor,
                .mesh = mesh_handle,
                .submesh_index = sub_idx,
                .record = InstanceSlot{transform_id.id, material_resource->data_id}
            });

            const auto sort_key = MakeSortKey(scene.index,
                                              material_type,
                                              material_resource->descriptor,
                                              mesh_handle,
                                              sub_idx);

            sort_items_[topo_idx].push_back(SortItem{
                .key = sort_key,
                .payload_index = payload_idx
            });
        }
    });

    // sort buckets and emit batches
    const auto topo_count = sort_items_.size();
    for (std::size_t topo_idx = 0; topo_idx < topo_count; ++topo_idx)
    {
        auto& items = sort_items_[topo_idx];
        if (items.empty()) continue;

        const auto& payloads = payload_items_[topo_idx];

        RadixSort64(items, sort_temp_);

        std::uint32_t current_scene = ~0u;
        std::uint32_t scene_batch_start = 0;
        std::uint64_t last_batch_key = std::numeric_limits<std::uint64_t>::max();

        for (const auto& item : items)
        {
            const auto scene_idx = static_cast<std::uint32_t>((item.key >> 60) & 0xFULL);
            const auto& payload = payloads[item.payload_index];

            // close previous scene batch range
            if (scene_idx != current_scene)
            {
                if (current_scene < MaxActiveScenes)
                {
                    scenes_[current_scene].topology_ranges[topo_idx] = BatchRange{
                        .offset = scene_batch_start,
                        .count = static_cast<std::uint32_t>(batches_.size()) - scene_batch_start
                    };
                }

                current_scene = scene_idx;
                scene_batch_start = static_cast<std::uint32_t>(batches_.size());
                last_batch_key = std::numeric_limits<std::uint64_t>::max();
            }

            // merge adjacent instances if sort keys match
            const bool can_merge = (batches_.size() > scene_batch_start) && (item.key == last_batch_key);

            if (!can_merge)
            {
                SubmissionBatch batch;
                batch.type = payload.type;
                batch.descriptor = payload.descriptor;
                batch.mesh = payload.mesh;
                batch.submesh_index = payload.submesh_index;
                batch.instance_offset = static_cast<std::uint32_t>(instances_.size());
                batch.instance_count = 0;
                batches_.push_back(batch);

                last_batch_key = item.key;
            }

            instances_.push_back(payload.record);
            ++batches_.back().instance_count;
        }

        // close trailing scene batch range
        if (current_scene < MaxActiveScenes)
        {
            scenes_[current_scene].topology_ranges[topo_idx] = BatchRange{
                .offset = scene_batch_start,
                .count = static_cast<std::uint32_t>(batches_.size()) - scene_batch_start
            };
        }
    }
}
