//
// Created by SherSpidey on 2026/8/26.
//

#ifndef NEXENGINE_RENDERUTILS_H
#define NEXENGINE_RENDERUTILS_H

#include <flecs.h>

#include <NexEngine/Core/MathUtils.h>

#include <NexEngine/Geometry/Primitives.h>

#include <NexEngine/Renderer/Render/RenderObject.h>
#include <NexEngine/Renderer/Render/RenderBuffer.h>
#include <NexEngine/Renderer/Render/RenderRuntime.h>
#include <NexEngine/Renderer/Pipeline/PipelineRegistry.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

namespace Nex
{
    inline PersistentSlotIndex AllocateTransform(RenderRuntime& runtime)
    {
        return runtime.buffer_manager->Acquire(runtime.transform_table);
    }

    inline void UpdateTransform(RenderRuntime& runtime, PersistentSlotIndex id, const Transform& transform)
    {
        runtime.buffer_manager->Write(runtime.transform_table, id, &transform);
    }

    inline void ReleaseTransform(RenderRuntime& runtime, PersistentSlotIndex id)
    {
        runtime.buffer_manager->Release(runtime.transform_table, id);
    }

    inline void UpdateTransform(RenderRuntime& runtime,
                                const Transform& transform,
                                const TransformID& id,
                                const PrimitiveScale* scale)
    {
        auto final_transform = transform;
        if (scale) final_transform.data = ApplyLocalScale(final_transform.data, scale->scale);

        UpdateTransform(runtime, id.id, final_transform);
    }

    inline void UpdateTransform(RenderRuntime& runtime, flecs::entity entity)
    {
        const auto transform = entity.try_get<Transform>();
        const auto id = entity.try_get<TransformID>();
        const auto scale = entity.try_get<PrimitiveScale>();

        if (!transform || !id) return;

        UpdateTransform(runtime, *transform, *id, scale);
    }

    inline const GraphicsPipeline* ResolveForwardPipeline(PipelineRegistry& registry,
                                                          MaterialType material_type,
                                                          Diligent::TEXTURE_FORMAT color_format,
                                                          Diligent::TEXTURE_FORMAT depth_format,
                                                          RenderTopoFlag topology = RenderTopoFlag::Triangle)
    {
        if (material_type == MaterialType::None) return nullptr;

        const TechniqueDesc desc
        {
            RenderPassType::Forward,
            material_type,
            topology
        };

        const auto technique_id = registry.Declare(desc);
        if (technique_id == InvalidTechniqueID) return nullptr;

        const PipelineKey key
        {
            technique_id,
            color_format,
            depth_format
        };

        return registry.Acquire(key);
    }
}

#endif //NEXENGINE_RENDERUTILS_H
