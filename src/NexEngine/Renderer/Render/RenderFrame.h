//
// Created by SherSpidey on 2026/8/25.
//

#ifndef NEXENGINE_RENDERFRAME_H
#define NEXENGINE_RENDERFRAME_H

#include <array>
#include <vector>

#include <flecs.h>

#include <NexEngine/Core/Scene.h>

#include <NexEngine/Geometry/Mesh.h>

#include <NexEngine/Renderer/Renderer.h>
#include <NexEngine/Renderer/Camera/Camera.h>
#include <NexEngine/Renderer/Device/Surface.h>
#include <NexEngine/Renderer/Material/Material.h>
#include <NexEngine/Renderer/Render/RenderBuffer.h>
#include <NexEngine/Renderer/Render/RenderObject.h>

namespace Nex
{
    struct ResourceManager;

    struct RenderViewItem
    {
        SurfaceHandle surface{};
        std::uint32_t order = 0;

        Viewport viewport{};
        RenderTopoFlag view_mask = RenderTopoFlag::Triangle;

        std::uint8_t clear_flags = 0;
        Vector4 clear_color{0.350f, 0.350f, 0.350f, 1.0f};
        float clear_depth = 1.0f;
        std::uint8_t clear_stencil = 0;

        Matrix4 view_matrix = IdentityMatrix4();
        Matrix4 projection_matrix = IdentityMatrix4();
        Vector3 camera_position{0.0f, 0.0f, 0.0f};

        std::uint32_t scene_index = 0;
    };

    // intermediate 16-byte sorting proxy for radix sort cache efficiency
    struct SortItem
    {
        std::uint64_t key = 0;
        std::uint32_t payload_index = 0;
    };

    // instance payload preserved in-place during sorting
    struct InstanceItem
    {
        MaterialType type = MaterialType::None;
        DescriptorHandle descriptor{};
        MeshHandle mesh{};
        std::uint32_t submesh_index = 0;
        InstanceSlot record{};
    };

    // instanced draw call command with shared pipeline states
    struct SubmissionBatch
    {
        MaterialType type;
        DescriptorHandle descriptor;
        MeshHandle mesh;
        std::uint32_t submesh_index;

        std::uint32_t instance_offset;
        std::uint32_t instance_count;
    };

    struct BatchRange
    {
        std::uint32_t offset = 0;
        std::uint32_t count = 0;
    };

    struct SceneLightingItem
    {
        EnvironmentConstants environment{};
        DirectionalLightConstants sun{};
        std::uint32_t local_light_count = 0;
    };

    struct SceneMetadata
    {
        SceneLightingItem lighting{};
        std::array<BatchRange, raw(MeshTopology::Count)> topology_ranges{};
    };

    struct RenderFrame
    {
        explicit RenderFrame(flecs::world& world);

        RenderFrame(const RenderFrame&) = delete;
        RenderFrame& operator=(const RenderFrame&) = delete;
        RenderFrame(RenderFrame&&) noexcept = default;
        RenderFrame& operator=(RenderFrame&&) noexcept = default;

        ArrayView<const RenderViewItem> RenderViews() const
        {
            return {render_views_.data(), render_views_.size()};
        }

        ArrayView<const InstanceSlot> Instances() const
        {
            return {instances_.data(), instances_.size()};
        }

        ArrayView<const SubmissionBatch> SubmissionBatches(std::uint32_t scene_index, MeshTopology type) const
        {
            if (scene_index >= MaxActiveScenes) return {};

            const auto idx = raw(type);
            if (idx >= raw(MeshTopology::Count)) return {};

            const auto& [offset, count] = scenes_[scene_index].topology_ranges[idx];
            if (count == 0 || offset >= batches_.size()) return {};

            return {batches_.data() + offset, count};
        }

        ArrayView<const SubmissionBatch> SubmissionBatches(MeshTopology type) const
        {
            return SubmissionBatches(0, type);
        }

        const SceneLightingItem& SceneLighting(std::uint32_t scene_index) const
        {
            if (scene_index >= MaxActiveScenes) return scenes_[0].lighting;
            return scenes_[scene_index].lighting;
        }

        void Extract(flecs::world&);

    private:
        void ExtractSceneLighting();
        void ExtractRenderViews();
        void ExtractInstances(flecs::world&);

        flecs::query<const Scene> scene_query_;
        flecs::query<const Camera, const Transform, const RenderView, const Scene*> camera_query_;
        flecs::query<const MeshHandle, const TransformID, const Renderer, const MaterialHandle*, const Scene>
        instance_query_;

        std::vector<RenderViewItem> render_views_;
        std::vector<InstanceSlot> instances_;
        std::vector<SubmissionBatch> batches_;

        std::array<SceneMetadata, MaxActiveScenes> scenes_;
        std::array<std::vector<SortItem>, raw(MeshTopology::Count)> sort_items_;
        std::array<std::vector<InstanceItem>, raw(MeshTopology::Count)> payload_items_;
        std::vector<SortItem> sort_temp_;
    };
}

#endif //NEXENGINE_RENDERFRAME_H
