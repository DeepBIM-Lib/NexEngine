#include <queue>
#include <unordered_set>

#include <NexEngine/Renderer/Pipeline/FramePlan.h>

using namespace Nex;

FramePlanBuilder& FramePlanBuilder::DeclareResource(FrameResourceID id, bool external)
{
    plan_.resources.push_back({.id = std::move(id), .external = external});
    return *this;
}

FramePlanBuilder& FramePlanBuilder::AddPass(FramePass pass)
{
    plan_.passes.push_back(std::move(pass));
    return *this;
}

CompiledFramePlan FramePlanBuilder::Compile() const
{
    return Compile(plan_);
}

CompiledFramePlan FramePlanBuilder::Compile(const FramePlan& plan)
{
    CompiledFramePlan result;
    auto num_pass = plan.passes.size();

    // resource duplication check
    std::unordered_map<FrameResourceID, bool> resources;
    for (const auto& resource : plan.resources)
    {
        if (resource.id.empty() || !resources.emplace(resource.id, resource.external).second)
        {
            result.error = "frame plan has a duplicate or empty resource id";
            return result;
        }
    }

    // pass duplication & resource dependency check
    std::unordered_map<FramePassID, std::size_t> pass_indices;
    for (std::size_t i = 0; i < num_pass; ++i)
    {
        const auto& pass = plan.passes[i];
        if (pass.id.empty() || !pass_indices.emplace(pass.id, i).second)
        {
            result.error = "frame plan has a duplicate or empty pass id";
            return result;
        }
        for (const auto& resource : pass.reads)
        {
            if (!resources.contains(resource))
            {
                result.error = "frame pass reads an undeclared resource: " + resource;
                return result;
            }
        }
        for (const auto& resource : pass.writes)
        {
            if (!resources.contains(resource))
            {
                result.error = "frame pass writes an undeclared resource: " + resource;
                return result;
            }
        }
    }

    /*-- DAG building --*/
    
    // adjacency building
    std::vector<std::size_t> incoming(num_pass);
    std::vector<std::vector<std::size_t>> outgoing(num_pass);
    for (std::size_t i = 0; i < num_pass; ++i)
    {
        std::unordered_set<std::size_t> unique_dependencies;
        for (const auto& dependency_id : plan.passes[i].dependencies)
        {
            const auto dependency = pass_indices.find(dependency_id);
            if (dependency == pass_indices.end())
            {
                result.error = "frame pass has a missing dependency: " + dependency_id;
                return result;
            }
            if (dependency->second == i || !unique_dependencies.insert(dependency->second).second)
            {
                result.error = "frame pass has a duplicate or self dependency: " + dependency_id;
                return result;
            }

            outgoing[dependency->second].push_back(i);
            ++incoming[i];
        }
    }

    std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<>> ready;
    for (std::size_t i = 0; i < incoming.size(); ++i)
    {
        // adding starter node
        if (incoming[i] == 0)
        {
            ready.push(i);
        }
    }

    // topological sort
    while (!ready.empty())
    {
        const auto current = ready.top();
        ready.pop();
        result.order.push_back(current);

        for (const auto next : outgoing[current])
        {
            if (--incoming[next] == 0)
            {
                ready.push(next);
            }
        }
    }

    if (result.order.size() != plan.passes.size())
    {
        result.order.clear();
        result.error = "frame pass dependency cycle";
    }

    return result;
}
