//
// Created by SherSpidey on 2026/7/10.
//

#ifndef NEXENGINE_MESHRESOURCE_H
#define NEXENGINE_MESHRESOURCE_H

#include <cstdint>

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsTools/interface/VertexPool.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/BufferSuballocator.h>

#include <NexEngine/Geometry/MeshData.h>

namespace Nex
{
    inline bool operator==(const MeshSubmesh& lhs, const MeshSubmesh& rhs) noexcept
    {
        return lhs.topology == rhs.topology &&
            lhs.index_offset == rhs.index_offset &&
            lhs.index_count == rhs.index_count &&
            lhs.vertex_offset == rhs.vertex_offset;
    }

    inline bool operator==(const MeshData& lhs, const MeshData& rhs) noexcept
    {
        if (lhs.positions.size() != rhs.positions.size() ||
            lhs.normals.size() != rhs.normals.size() ||
            lhs.tangents.size() != rhs.tangents.size() ||
            lhs.uv0.size() != rhs.uv0.size() ||
            lhs.uv1.size() != rhs.uv1.size() ||
            lhs.colors.size() != rhs.colors.size() ||
            lhs.indices.size() != rhs.indices.size() ||
            lhs.submeshes.size() != rhs.submeshes.size())
            return false;

        const auto has_canonical_streams = [](const MeshData& data) noexcept
        {
            const auto count = data.positions.size();
            return data.normals.size() == count &&
                data.tangents.size() == count &&
                data.uv0.size() == count &&
                data.uv1.size() == count &&
                data.colors.size() == count;
        };

        if (!has_canonical_streams(lhs) || !has_canonical_streams(rhs)) return false;

        for (std::size_t i = 0; i < lhs.positions.size(); ++i)
        {
            if (lhs.positions[i] != rhs.positions[i] ||
                lhs.normals[i] != rhs.normals[i] ||
                lhs.tangents[i] != rhs.tangents[i] ||
                lhs.uv0[i] != rhs.uv0[i] ||
                lhs.uv1[i] != rhs.uv1[i] ||
                lhs.colors[i] != rhs.colors[i])
                return false;
        }

        for (std::size_t i = 0; i < lhs.indices.size(); ++i)
        {
            if (lhs.indices[i] != rhs.indices[i]) return false;
        }

        for (std::size_t i = 0; i < lhs.submeshes.size(); ++i)
        {
            const auto& a = lhs.submeshes[i];
            const auto& b = rhs.submeshes[i];

            if (a != b) return false;
        }

        return true;
    }
}

template <>
struct std::hash<Nex::MeshData>
{
    std::size_t operator()(const Nex::MeshData& data) const noexcept
    {
        Nex::StableHash64Builder hash;

        hash.AppendSize(data.positions.size());

        for (const auto& position : data.positions)
            Nex::HashAppend(hash, position);

        hash.AppendSize(data.normals.size());
        for (const auto& normal : data.normals)
            Nex::HashAppend(hash, normal);

        hash.AppendSize(data.tangents.size());
        for (const auto& tangent : data.tangents)
            Nex::HashAppend(hash, tangent);

        hash.AppendSize(data.uv0.size());
        for (const auto& uv : data.uv0)
            Nex::HashAppend(hash, uv);

        hash.AppendSize(data.uv1.size());
        for (const auto& uv : data.uv1)
            Nex::HashAppend(hash, uv);

        hash.AppendSize(data.colors.size());
        for (const auto& color : data.colors)
            Nex::HashAppend(hash, color);

        hash.AppendSize(data.indices.size());
        for (const auto value : data.indices)
            hash.Append(value);

        hash.AppendSize(data.submeshes.size());
        for (const auto& [topology, index_offset,
                 index_count, vertex_offset] : data.submeshes)
        {
            hash.Append(topology);
            hash.Append(index_offset);
            hash.Append(index_count);
            hash.Append(vertex_offset);
        }

        return hash.Finish();
    }
};

namespace Nex
{
    struct MeshResource
    {
        MeshData data;

        Hash64 hash;
        std::uint32_t ref_count;
        bool persistent;

        Diligent::RefCntAutoPtr<Diligent::IVertexPoolAllocation> vertex_allocation;
        Diligent::RefCntAutoPtr<Diligent::IBufferSuballocation> index_allocation;

        explicit MeshResource(MeshData desc, std::uint64_t desc_hash) noexcept
            : data{std::move(desc)},
              hash{desc_hash},
              ref_count{1},
              persistent{false}
        {
        }

        MeshResource(const MeshResource&) = delete;
        MeshResource& operator=(const MeshResource&) = delete;
        MeshResource(MeshResource&&) noexcept = default;
        MeshResource& operator=(MeshResource&&) noexcept = default;

        std::uint32_t GetBaseVertex() const noexcept
        {
            return vertex_allocation ? vertex_allocation->GetStartVertex() : 0;
        }

        Diligent::IBuffer* GetVertexBuffer(std::uint32_t stream) const noexcept
        {
            return vertex_allocation ? vertex_allocation->GetBuffer(stream) : nullptr;
        }

        std::uint32_t GetFirstIndex() const noexcept
        {
            return index_allocation ? index_allocation->GetOffset() / sizeof(std::uint32_t) : 0;
        }

        Diligent::IBuffer* GetIndexBuffer() const noexcept
        {
            return index_allocation ? index_allocation->GetBuffer() : nullptr;
        }
    };
}

#endif //NEXENGINE_MESHRESOURCE_H
