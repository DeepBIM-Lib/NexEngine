#ifndef NEXENGINE_FRAMEPLAN_H
#define NEXENGINE_FRAMEPLAN_H

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <NexEngine/Core/IDTypes.h>

namespace Nex
{
    struct FrameResource
    {
        FrameResourceID id;
        bool external = false;
    };

    struct FramePass
    {
        FramePassID id;
        std::string name;
        std::vector<FrameResourceID> reads;
        std::vector<FrameResourceID> writes;
        std::vector<FramePassID> dependencies;
    };

    struct FramePlan
    {
        std::vector<FrameResource> resources;
        std::vector<FramePass> passes;
    };

    struct CompiledFramePlan
    {
        std::string error;
        std::vector<std::size_t> order;

        explicit operator bool() const noexcept
        {
            return error.empty();
        }
    };

    template <typename Context>
    struct CompiledFramePipeline
    {
        std::string error;
        std::vector<std::function<void(Context&)>> executors;

        explicit operator bool() const noexcept
        {
            return error.empty();
        }

        void Execute(Context& context) const
        {
            for (const auto& execute : executors)
                execute(context);
        }
    };

    class FramePlanBuilder
    {
    public:
        FramePlanBuilder& DeclareResource(FrameResourceID, bool external = false);
        FramePlanBuilder& AddPass(FramePass);

        CompiledFramePlan Compile() const;
        static CompiledFramePlan Compile(const FramePlan& plan);

        const FramePlan& GetFramePlan() const noexcept
        {
            return plan_;
        }

    private:
        FramePlan plan_;
    };

    template <typename Context>
    class FramePassRegistry
    {
    public:
        using Executor = std::function<void(Context&)>;

        bool Register(FramePassID id, Executor executor)
        {
            return executors_.emplace(std::move(id), std::move(executor)).second;
        }

        CompiledFramePipeline<Context> Compile(const FramePlan& plan) const
        {
            CompiledFramePipeline<Context> result;
            
            const auto compiled_plan = FramePlanBuilder::Compile(plan);
            if (!compiled_plan)
            {
                result.error = compiled_plan.error;
                return result;
            }
            
            result.executors.reserve(compiled_plan.order.size());
            for (const auto pass_index : compiled_plan.order)
            {
                const auto& pass = plan.passes[pass_index];
                const auto found = executors_.find(pass.id);
                if (found == executors_.end())
                {
                    result.executors.clear();
                    result.error = "frame pass has no registered executor: " + pass.id;
                    return result;
                }
                result.executors.push_back(found->second);
            }
            
            return result;
        }

    private:
        std::unordered_map<FramePassID, Executor> executors_;
    };
}
#endif
