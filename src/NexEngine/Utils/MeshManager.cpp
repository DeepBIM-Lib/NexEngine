//
// Created by SherSpidey on 2026/7/10.
//

#include <algorithm>
#include <cstddef>
#include <limits>

#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Platforms/Basic/interface/DebugUtilities.hpp>

#include <NexEngine/Geometry/MeshResource.h>
#include <NexEngine/Geometry/Primitives/SpherifiedCube.h>

#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/InputDataUtils.h>

#include <NexEngine/Utils/MeshManager.h>

using namespace Nex;
using namespace Diligent;

namespace
{
    constexpr std::uint32_t StandardVertexCapacity = 64u * 1024u;
    constexpr std::uint32_t StandardIndexCapacity = 192u * 1024u;
    constexpr std::uint32_t PageGranularity = 4096u;

    // clamped to 2GB for all backends to satisfy D3D11 buffer limits
    constexpr std::uint64_t MaxBufferSize = (std::uint64_t{1} << 31) - 1;

    constexpr float ByteToFloat = 1.0f / 255.0f;

    static_assert(sizeof(PositionSlot) == 12);
    static_assert(sizeof(AttributeSlot) == 36);
    static_assert(offsetof(AttributeSlot, normal) == 0);
    static_assert(offsetof(AttributeSlot, uv0) == 12);
    static_assert(offsetof(AttributeSlot, tangent) == 20);
    static_assert(sizeof(UberSlot) == 24);

    constexpr VertexPoolElementDesc elements[]
    {
        VertexPoolElementDesc{sizeof(PositionSlot), BIND_VERTEX_BUFFER},
        VertexPoolElementDesc{sizeof(AttributeSlot), BIND_VERTEX_BUFFER},
        VertexPoolElementDesc{sizeof(UberSlot), BIND_VERTEX_BUFFER}
    };

    std::uint32_t AlignCapacity(std::uint32_t value) noexcept
    {
        const auto aligned =
            (static_cast<std::uint64_t>(value) + PageGranularity - 1) / PageGranularity * PageGranularity;

        return static_cast<std::uint32_t>((std::min)(
            aligned, static_cast<std::uint64_t>((std::numeric_limits<std::uint32_t>::max)())));
    }

    template <typename Shape, typename... Args>
    MeshHandle CreateAffinePrimitive(MeshManager& manager,
                                     ResourceManager& resource_manager,
                                     const char* cache_key,
                                     Args&&... args)
    {
        Shape shape;
        Primitives::Generate(shape, std::forward<Args>(args)...);

        MeshDesc desc;
        desc.data = std::move(shape.data);
        desc.cache_key = Nex::String{cache_key};

        auto handle = manager.Create(std::move(desc), resource_manager);
        auto mesh = resource_manager.GetResource(handle);
        mesh->persistent = true;

        return handle;
    }

    bool CreateParametricPrimitives(PrimitiveType type, const void* data, MeshDesc& desc)
    {
        if (IsAffine(type)) return false;

        // do creation stuff here

        return true;
    }
}

MeshManager::MeshManager(IRenderDevice* device, IDeviceContext* context, ResourceManager& resource_manager)
    : device_{device},
      context_{context}
{
    CreatePage(StandardVertexCapacity, StandardIndexCapacity);
    CreateAffinePrimitives(resource_manager);
}

MeshHandle MeshManager::Create(MeshDesc desc, ResourceManager& resource_manager)
{
    const auto validation = SanitizeMeshData(desc);
    if (!validation)
    {
        LOG_ERROR_MESSAGE(
            "CreateMesh validation failed: ", ToString(validation.code),
            " (stream=", ToString(validation.stream), ", element=", validation.element_index,
            ", submesh=", validation.submesh_index, ")");
        return MakeInvalidHandle<MeshResource>();
    }

    const auto hash = std::hash<MeshData>{}(desc.data);

    std::string cache_key;
    if (!desc.cache_key.empty())
        cache_key.assign(desc.cache_key.data(), desc.cache_key.size());

    if (!cache_key.empty())
    {
        if (const auto key_it = key_cache_.find(cache_key); key_it != key_cache_.end())
        {
            auto resource = resource_manager.GetResource(key_it->second);
            if (!resource || resource->hash != hash || resource->data != desc.data)
            {
                LOG_ERROR_MESSAGE("CreateMesh cache key '", cache_key, "' already identifies different content");
                return MakeInvalidHandle<MeshResource>();
            }

            const auto bucket_it = mesh_cache_.find(hash);
            if (bucket_it != mesh_cache_.end())
            {
                for (const auto& handle : bucket_it->second)
                {
                    if (handle != key_it->second)
                        continue;

                    if (!resource_manager.Retain(handle))
                    {
                        LOG_ERROR_MESSAGE("CreateMesh reference count overflow");
                        return MakeInvalidHandle<MeshResource>();
                    }

                    return handle;
                }
            }

            LOG_ERROR_MESSAGE("CreateMesh cache ownership is inconsistent");
            return MakeInvalidHandle<MeshResource>();
        }
    }

    if (const auto it = mesh_cache_.find(hash); it != mesh_cache_.end())
    {
        for (auto& handle : it->second)
        {
            const auto resource = resource_manager.GetResource(handle);
            if (resource && resource->data == desc.data)
            {
                if (!resource_manager.Retain(handle))
                {
                    LOG_ERROR_MESSAGE("CreateMesh reference count overflow");
                    return MakeInvalidHandle<MeshResource>();
                }

                if (!cache_key.empty()) key_cache_.emplace(cache_key, handle);

                return handle;
            }
        }
    }

    // try to allocate and upload mesh
    Allocation allocation;
    if (!Allocate(static_cast<std::uint32_t>(desc.data.positions.size()),
                  static_cast<std::uint32_t>(desc.data.indices.size()),
                  allocation) ||
        !Upload(desc.data, allocation))
    {
        LOG_ERROR_MESSAGE("CreateMesh failed to allocate or upload shared GPU storage");
        return MakeInvalidHandle<MeshResource>();
    }

    // create mesh
    MeshResource mesh(std::move(desc.data), hash);

    // bind resource
    mesh.vertex_allocation = std::move(allocation.vertices);
    mesh.index_allocation = std::move(allocation.indices);

    // create handle
    auto mesh_handle = resource_manager.CreateResource<MeshResource>(std::move(mesh));

    // add to cache
    if (!cache_key.empty()) key_cache_.emplace(cache_key, mesh_handle);
    mesh_cache_[hash].emplace_back(mesh_handle);

    return mesh_handle;
}

MeshHandle MeshManager::CreatePrimitive(PrimitiveType type,
                                        const void* data,
                                        ResourceManager& resource_manager)
{
    if (IsAffine(type))
    {
        auto& handle = primitive_meshes_[raw(type)];
        if (!resource_manager.Retain(handle))
        {
            LOG_ERROR_MESSAGE("CreateMesh reference count overflow");
            return MakeInvalidHandle<MeshResource>();
        }
        return handle;
    }

    // create parametric primitives
    MeshDesc desc;
    CreateParametricPrimitives(type, data, desc);

    return Create(std::move(desc), resource_manager);
}

bool MeshManager::Release(MeshHandle handle,
                          ResourceManager& resource_manager,
                          std::uint64_t serial,
                          bool& retired)
{
    retired = false;

    auto mesh_resource = resource_manager.GetResource(handle);
    if (!mesh_resource) return false;

    // can not release a builtin resource
    if (mesh_resource->ref_count == 1 && mesh_resource->persistent) return false;

    if (mesh_resource->ref_count > 1)
    {
        --mesh_resource->ref_count;
        return true;
    }

    // time to release mesh resource
    if (!resource_manager.DestroyResource(handle, serial)) return false;

    // clean cache
    if (auto it = mesh_cache_.find(mesh_resource->hash); it != mesh_cache_.end())
    {
        std::erase_if(it->second, [&](const MeshHandle& mesh_handle)
        {
            return handle == mesh_handle;
        });

        if (it->second.empty()) mesh_cache_.erase(it);
    }
    
    for (auto it = key_cache_.begin(); it != key_cache_.end();)
    {
        if (it->second == handle)
            it = key_cache_.erase(it);
        else
            ++it;
    }

    retired = true;
    return true;
}

bool MeshManager::CreatePage(std::uint32_t vertex_capacity, std::uint32_t index_capacity, bool dedicated)
{
    if (vertex_capacity == 0 ||
        index_capacity == 0 ||
        static_cast<std::uint64_t>(vertex_capacity) * sizeof(AttributeSlot) > MaxBufferSize ||
        static_cast<std::uint64_t>(index_capacity) * sizeof(std::uint32_t) > MaxBufferSize)
        return false;

    VertexPoolCreateInfo vertex_info;
    vertex_info.Desc.Name = "Nex Mesh Vertex Page";
    vertex_info.Desc.pElements = elements;
    vertex_info.Desc.NumElements = static_cast<Uint32>(std::size(elements));
    vertex_info.Desc.VertexCount = vertex_capacity;
    vertex_info.ExtraVertexCount = 0;
    vertex_info.MaxVertexCount = vertex_capacity;

    BufferSuballocatorCreateInfo index_info;
    index_info.Desc.Name = "Nex Mesh Index Page";
    index_info.Desc.Size = static_cast<Uint64>(index_capacity) * sizeof(std::uint32_t);
    index_info.Desc.BindFlags = BIND_INDEX_BUFFER;
    index_info.MaxSize = index_info.Desc.Size;

    BufferPage page;
    CreateVertexPool(device_, vertex_info, &page.vertex_pool);
    CreateBufferSuballocator(device_, index_info, &page.index_allocator);

    if (!page.vertex_pool ||
        !page.index_allocator ||
        !page.vertex_pool->GetBuffer(0) ||
        !page.vertex_pool->GetBuffer(1) ||
        !page.vertex_pool->GetBuffer(2) ||
        !page.index_allocator->GetBuffer())
        return false;

    page.dedicated = dedicated;
    pages_.emplace_back(std::move(page));

    return true;
}

bool MeshManager::Allocate(std::uint32_t vertex_count, std::uint32_t index_count, Allocation& allocation)
{
    const auto index_bytes = static_cast<std::uint64_t>(index_count) * sizeof(std::uint32_t);

    if (index_bytes > MaxBufferSize ||
        static_cast<std::uint64_t>(vertex_count) * sizeof(AttributeSlot) > MaxBufferSize)
        return false;

    auto try_page = [&](std::uint32_t page_index) -> bool
    {
        auto& page = pages_[page_index];

        if (page.dedicated)
        {
            // dedicated pages are reserved exclusively for oversized allocations
            if (vertex_count <= StandardVertexCapacity && index_count <= StandardIndexCapacity) return false;

            VertexPoolUsageStats usage;
            page.vertex_pool->GetUsageStats(usage);
            // dedicated pages must be completely empty before use
            if (usage.AllocationCount != 0) return false;
        }

        page.vertex_pool->Allocate(vertex_count, &allocation.vertices);
        if (!allocation.vertices) return false;

        page.index_allocator->Allocate(
            static_cast<Uint32>(index_bytes), alignof(std::uint32_t), &allocation.indices);
        if (!allocation.indices)
        {
            // roll back vertex allocation if index allocation fails
            allocation.vertices.Release();
            return false;
        }

        allocation.page_index = page_index;
        return true;
    };

    // attempt allocation across existing pages
    for (std::uint32_t i = 0; i < pages_.size(); ++i)
    {
        if (try_page(i)) return true;
    }

    // determine required capacity and create a new page
    const auto vertex_capacity = (std::max)(StandardVertexCapacity, AlignCapacity(vertex_count));
    const auto index_capacity = (std::max)(StandardIndexCapacity, AlignCapacity(index_count));

    const bool dedicated = vertex_count > StandardVertexCapacity || index_count > StandardIndexCapacity;

    if (!CreatePage(vertex_capacity, index_capacity, dedicated)) return false;

    // allocate from the newly appended page
    return try_page(static_cast<std::uint32_t>(pages_.size() - 1));
}

bool MeshManager::Upload(const MeshData& data, Allocation& allocation) const
{
    std::vector<PositionSlot> positions(data.positions.size());
    std::vector<AttributeSlot> attributes(data.positions.size());
    std::vector<UberSlot> uber(data.positions.size());

    for (std::size_t i = 0; i < data.positions.size(); ++i)
    {
        positions[i] = {.position = data.positions[i]};
        attributes[i] = {
            .normal = data.normals[i],
            .uv0 = data.uv0[i],
            .tangent = data.tangents[i]
        };
        uber[i] = {
            .uv1 = data.uv1[i],
            .color = {
                .x = static_cast<float>(data.colors[i].r) * ByteToFloat,
                .y = static_cast<float>(data.colors[i].g) * ByteToFloat,
                .z = static_cast<float>(data.colors[i].b) * ByteToFloat,
                .w = static_cast<float>(data.colors[i].a) * ByteToFloat
            }
        };
    }

    auto& page = pages_[allocation.page_index];

    page.vertex_pool->UpdateAll(device_, context_);
    auto index_buffer = page.index_allocator->Update(device_, context_);
    auto position_buffer = page.vertex_pool->GetBuffer(0);
    auto attribute_buffer = page.vertex_pool->GetBuffer(1);
    auto uber_buffer = page.vertex_pool->GetBuffer(2);
    if (!position_buffer || !attribute_buffer || !uber_buffer || !index_buffer) return false;

    const auto start_vertex = allocation.vertices->GetStartVertex();

    // `VertexPool` offsets are in vertices while `BufferSuballocator` offsets are bytes
    // upload each SoA stream at the same vertex base, and indices at their byte offset
    context_->UpdateBuffer(
        position_buffer,
        static_cast<Uint64>(start_vertex) * sizeof(PositionSlot),
        positions.size() * sizeof(PositionSlot),
        positions.data(),
        RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    context_->UpdateBuffer(
        attribute_buffer,
        static_cast<Uint64>(start_vertex) * sizeof(AttributeSlot),
        attributes.size() * sizeof(AttributeSlot),
        attributes.data(),
        RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    context_->UpdateBuffer(
        uber_buffer,
        static_cast<Uint64>(start_vertex) * sizeof(UberSlot),
        uber.size() * sizeof(UberSlot),
        uber.data(),
        RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    context_->UpdateBuffer(
        index_buffer,
        allocation.indices->GetOffset(),
        data.indices.size() * sizeof(std::uint32_t),
        data.indices.data(),
        RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    return true;
}

void MeshManager::CreateAffinePrimitives(ResourceManager& resource_manager)
{
    primitive_meshes_[raw(PrimitiveType::Cube)] = CreateAffinePrimitive<Primitives::Cube>(
        *this, resource_manager, "builtin:cube"
    );
    primitive_meshes_[raw(PrimitiveType::QuadSphere)] = CreateAffinePrimitive<Primitives::QuadSphere>(
        *this, resource_manager, "builtin:quad-sphere"
    );
}
