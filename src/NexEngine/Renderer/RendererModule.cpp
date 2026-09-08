//
// Created by SherSpidey on 2026/7/9.
//

#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>

#include <NexEngine/Renderer/Renderer.h>

#include <NexEngine/Renderer/Device/Surface.h>
#include <NexEngine/Renderer/Device/SurfaceResource.h>

#include <NexEngine/Renderer/Render/RenderCore.h>
#include <NexEngine/Renderer/Render/RenderUtils.h>
#include <NexEngine/Renderer/Render/RenderFrame.h>
#include <NexEngine/Renderer/Render/RenderRuntime.h>
#include <NexEngine/Renderer/Render/RenderPipelineModule.h>

#include <NexEngine/Renderer/Pipeline/FramePlan.h>
#include <NexEngine/Renderer/Pipeline/PipelineRegistry.h>
#include <NexEngine/Renderer/Pipeline/RenderPassContext.h>

#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

#include <NexEngine/Renderer/RendererModule.h>

#include "NexEngine/Geometry/MeshResource.h"

using namespace Nex;
using namespace Diligent;

namespace
{
    template <typename Fn>
    void ForEachSurfaceGroup(const RenderViewItem* views, std::size_t count, Fn&& on_group)
    {
        std::size_t begin = 0;
        while (begin < count)
        {
            std::size_t end = begin + 1;
            while (end < count && views[end].surface == views[begin].surface)
                ++end;

            on_group(views + begin, end - begin);
            begin = end;
        }
    }

    const std::string PassClear = "clear";
    const std::string PassForward = "forward";
    const std::string PassPresent = "present";

    FramePlan BuildFramePlan()
    {
        FramePlanBuilder builder;

        builder.DeclareResource("view.color", true)
               .DeclareResource("view.depth", true)
               .AddPass({PassClear, "Clear", {}, {"view.color", "view.depth"}, {}})
               .AddPass({
                   PassForward, "Forward", {"view.color", "view.depth"},
                   {"view.color", "view.depth"}, {}
               })
               .AddPass({PassPresent, "Present", {"view.color", "view.depth"}, {}, {}});

        return builder.GetFramePlan();
    }

    void RegisterRenderer(flecs::world& world)
    {
        world.component<RenderTopoFlag>("RenderTopoFlag")
             .constant("Triangle", RenderTopoFlag::Triangle)
             .constant("Line", RenderTopoFlag::Line)
             .constant("Point", RenderTopoFlag::Point)
             .constant("All", RenderTopoFlag::All);

        world.component<Renderer>("Renderer")
             .member<RenderTopoFlag>("RenderType")
             .member<bool>("Visible");
    }

    void RegisterPipeline(flecs::world& world)
    {
        world.component<PipelineRegistry>("PipelineRegistry")
             .add(flecs::Singleton);
    }

    void RegisterRenderFrame(flecs::world& world)
    {
        FramePassRegistry<RenderPassContext> pass_registry;

        // clear pass registration
        pass_registry.Register(PassClear, [](RenderPassContext& ctx)
        {
            const auto [views, size] = ctx.render_frame->RenderViews();

            ForEachSurfaceGroup(views, size, [&](const RenderViewItem* group, std::size_t)
            {
                const auto surface = ctx.resource_manager->GetResource(group->surface);
                if (!surface) return;

                ctx.render_view = group;
                ctx.render_target_view = surface->GetRTV();
                ctx.depth_stencil_view = surface->GetDSV();
                ctx.color_format = surface->GetColorFormat();
                ctx.depth_format = surface->GetDepthFormat();

                ctx.context->SetRenderTargets(1,
                                              &ctx.render_target_view, ctx.depth_stencil_view,
                                              RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                if (group->clear_flags & raw(ClearFlag::Color))
                {
                    ctx.context->ClearRenderTarget(ctx.render_target_view, &group->clear_color,
                                                   RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                }
                if (group->clear_flags & raw(ClearFlag::Depth))
                {
                    ctx.context->ClearDepthStencil(ctx.depth_stencil_view, CLEAR_DEPTH_FLAG,
                                                   group->clear_depth, group->clear_stencil,
                                                   RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                }
            });
        });

        pass_registry.Register(PassForward, [](RenderPassContext& ctx)
        {
            const auto [views, size] = ctx.render_frame->RenderViews();

            ForEachSurfaceGroup(views, size, [&](const RenderViewItem* group, std::size_t count)
            {
                const auto surface = ctx.resource_manager->GetResource(group->surface);
                if (!surface) return;

                ctx.render_target_view = surface->GetRTV();
                ctx.depth_stencil_view = surface->GetDSV();
                ctx.color_format = surface->GetColorFormat();
                ctx.depth_format = surface->GetDepthFormat();

                ctx.context->SetRenderTargets(1,
                                              &ctx.render_target_view, ctx.depth_stencil_view,
                                              RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                
                for (std::size_t i = 0; i < count; ++i)
                {
                    const auto& view = group[i];
                    ctx.render_view = &view;

                    const Diligent::Viewport viewport
                    {
                        view.viewport.x,
                        view.viewport.y,
                        view.viewport.width,
                        view.viewport.height,
                        0.0f,
                        1.0f
                    };
                    ctx.context->SetViewports(1, &viewport, 0, 0);

                    const auto& scene_lighting = ctx.render_frame->SceneLighting(view.scene_index);
                    ctx.render_runtime->UploadFrameConstants(ctx.context, view, scene_lighting);
                    ctx.render_runtime->UpdateFrameBinding(*ctx.pipeline_registry);

                    IShaderResourceBinding* current_srb = nullptr;
                    for (const auto& batch :
                         ctx.render_frame->SubmissionBatches(view.scene_index, MeshTopology::TriangleList))
                    {
                        const auto pipeline = ResolveForwardPipeline(*ctx.pipeline_registry,
                                                                     batch.type,
                                                                     ctx.color_format,
                                                                     ctx.depth_format);
                        const auto mesh = ctx.resource_manager->GetResource(batch.mesh);
                        if (!pipeline || !mesh) continue;

                        const auto& submesh = mesh->data.submeshes[batch.submesh_index];

                        ctx.context->SetPipelineState(pipeline->state);

                        IBuffer* vertex_buffers[] = {
                            mesh->GetVertexBuffer(0),
                            mesh->GetVertexBuffer(1)
                        };
                        ctx.context->SetVertexBuffers(0, 2,
                                                      vertex_buffers, nullptr,
                                                      RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                                      SET_VERTEX_BUFFERS_FLAG_RESET);
                        ctx.context->SetIndexBuffer(mesh->GetIndexBuffer(), 0,
                                                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                        ctx.render_runtime->CommitBatch(ctx.context, batch.instance_offset);

                        const auto material_srb = ctx.material_cache->AcquireSRB
                        (
                            DescriptorSRBKey{batch.type, batch.descriptor},
                            pipeline->layout.material,
                            *ctx.buffer_manager,
                            *ctx.material_manager,
                            *ctx.resource_manager
                        );

                        if (material_srb != current_srb)
                        {
                            ctx.context->CommitShaderResources(material_srb,
                                                               RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                            
                            current_srb = material_srb;
                        }

                        DrawIndexedAttribs draw{};

                        draw.IndexType = VT_UINT32;
                        draw.NumIndices = submesh.index_count;
                        draw.FirstIndexLocation = mesh->GetFirstIndex() + submesh.index_offset;
                        draw.BaseVertex = mesh->GetBaseVertex() + submesh.vertex_offset;
                        draw.NumInstances = batch.instance_count;
                        draw.Flags = DRAW_FLAG_VERIFY_ALL;

                        ctx.context->DrawIndexed(draw);
                    }
                }
            });
        });

        // present pass registration
        pass_registry.Register(PassPresent, [](RenderPassContext& ctx)
        {
            const auto [views, size] = ctx.render_frame->RenderViews();

            ForEachSurfaceGroup(views, size, [&](const RenderViewItem* group, std::size_t)
            {
                const auto surface = ctx.resource_manager->GetResource(group->surface);
                if (surface && surface->swap_chain)
                {
                    surface->swap_chain->Present(0);
                }
            });
        });

        const auto frame_plan = BuildFramePlan();
        const auto compiled_pipeline = pass_registry.Compile(frame_plan);

        world.system("RenderFrame")
             .kind(flecs::PostFrame)
             .run([compiled_pipeline](flecs::iter& it)
             {
                 auto world = it.world();

                 const auto& render_core = world.get<RenderCore>();
                 auto& render_frame = world.get_mut<RenderFrame>();
                 auto& render_runtime = world.get_mut<RenderRuntime>();
                 auto& pipeline_registry = world.get_mut<PipelineRegistry>();
                 auto& material_cache = world.get_mut<MaterialBindingCache>();
                 auto& buffer_manager = world.get_mut<PersistentBufferManager>();
                 auto& material_manager = world.get_mut<MaterialManager>();
                 auto& resource_manager = world.get_mut<ResourceManager>();

                 auto& context = render_core.context;

                 buffer_manager.BeginFrame(context);

                 render_frame.Extract(world);
                 render_runtime.UploadInstances(context, render_frame.Instances());

                 RenderPassContext pass_ctx{};

                 pass_ctx.device = render_core.device;
                 pass_ctx.context = context;
                 pass_ctx.render_runtime = &render_runtime;
                 pass_ctx.pipeline_registry = &pipeline_registry;
                 pass_ctx.material_cache = &material_cache;
                 pass_ctx.buffer_manager = &buffer_manager;
                 pass_ctx.material_manager = &material_manager;
                 pass_ctx.resource_manager = &resource_manager;
                 pass_ctx.render_frame = &render_frame;

                 compiled_pipeline.Execute(pass_ctx);

                 buffer_manager.EndFrame(context);
             });
    }
}

RendererModule::RendererModule(flecs::world& world)
{
    world.module<RendererModule>("NexEngine::Renderer");

    world.component<RenderCore>("RenderCore")
         .add(flecs::Singleton);

    world.component<RenderRuntime>("RenderRuntime")
         .add(flecs::Singleton);

    world.component<RenderFrame>("RenderFrame")
         .add(flecs::Singleton);

    RegisterRenderer(world);

    RenderPipelineModule::Register(world);

    RegisterPipeline(world);

    // test code, the whole function will be replaced soon
    RegisterRenderFrame(world);
}
