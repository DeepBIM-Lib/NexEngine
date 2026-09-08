//
// Created by SherSpidey on 2026/7/10.
//

#include <limits>
#include <vector>
#include <algorithm>

#include <NexEngine/Utils/InputDataUtils.h>

using namespace Nex;

namespace
{
    struct WideVector3
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        WideVector3& operator+=(const WideVector3& rhs) noexcept
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }
    };

    WideVector3 operator-(const WideVector3& lhs, const WideVector3& rhs) noexcept
    {
        return {.x = lhs.x - rhs.x, .y = lhs.y - rhs.y, .z = lhs.z - rhs.z};
    }

    WideVector3 operator*(const WideVector3& value, double scale) noexcept
    {
        return {.x = value.x * scale, .y = value.y * scale, .z = value.z * scale};
    }

    double Dot(const WideVector3& lhs, const WideVector3& rhs) noexcept
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    WideVector3 Cross(const WideVector3& lhs, const WideVector3& rhs) noexcept
    {
        return {
            .x = lhs.y * rhs.z - lhs.z * rhs.y,
            .y = lhs.z * rhs.x - lhs.x * rhs.z,
            .z = lhs.x * rhs.y - lhs.y * rhs.x
        };
    }

    WideVector3 ToWide(const Vector3& value) noexcept
    {
        return {.x = value.x, .y = value.y, .z = value.z};
    }

    WideVector3 Difference(const Vector3& lhs, const Vector3& rhs) noexcept
    {
        return {
            .x = static_cast<double>(lhs.x) - rhs.x,
            .y = static_cast<double>(lhs.y) - rhs.y,
            .z = static_cast<double>(lhs.z) - rhs.z
        };
    }

    double MaxComponent(const WideVector3& value) noexcept
    {
        return (std::max)({std::abs(value.x), std::abs(value.y), std::abs(value.z)});
    }

    MeshDataResult Error(
        MeshDataErrorCode code,
        MeshDataStream stream,
        std::size_t element_index = 0,
        std::uint32_t submesh_index = 0) noexcept
    {
        return {
            .code = code,
            .stream = stream,
            .element_index = element_index,
            .submesh_index = submesh_index
        };
    }

    MaterialDataResult Error(MaterialDataErrorCode code) noexcept
    {
        return {.code = code,};
    }

    bool NormalizeDirection(const WideVector3& value, Vector3& normalized) noexcept
    {
        const auto maximum = MaxComponent(value);
        if (maximum == 0.0 || !std::isfinite(maximum))
            return false;

        const auto x = value.x / maximum;
        const auto y = value.y / maximum;
        const auto z = value.z / maximum;
        const auto inverse_length = 1.0 / std::sqrt(x * x + y * y + z * z);

        normalized = {
            .x = static_cast<float>(x * inverse_length),
            .y = static_cast<float>(y * inverse_length),
            .z = static_cast<float>(z * inverse_length)
        };

        return Finite(normalized);
    }

    Vector3 NormalizeDirection(const Vector3& value) noexcept
    {
        Vector3 normalized{};
        NormalizeDirection(ToWide(value), normalized);
        return normalized;
    }

    template <typename T>
    MeshDataResult ValidateStreamSize(const Array<T>& stream, std::size_t vertex_count, MeshDataStream name) noexcept
    {
        if (!stream.empty() && stream.size() != vertex_count)
            return Error(MeshDataErrorCode::AttributeSizeMismatch, name, stream.size());
        return {};
    }

    Vector3 StableTangent(const Vector3& normal) noexcept
    {
        const Vector3 axis = std::abs(normal.y) < 0.999f
                                 ? Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f}
                                 : Vector3{.x = 1.0f, .y = 0.0f, .z = 0.0f};
        return NormalizeDirection(cross(axis, normal));
    }

    constexpr std::uint32_t IndicesPerPrimitive(MeshTopology topology) noexcept
    {
        switch (topology)
        {
        case MeshTopology::TriangleList: return 3;
        case MeshTopology::LineList: return 2;
        case MeshTopology::PointList: return 1;
        case MeshTopology::Count: return 0;
        }
        return 0;
    }

    template <typename Func>
    void ForEachTriangle(const MeshData& data, Func&& func)
    {
        for (const auto& submesh : data.submeshes)
        {
            if (submesh.topology != MeshTopology::TriangleList)
                continue;

            const auto end = submesh.index_offset + submesh.index_count;
            for (std::uint32_t i = submesh.index_offset; i < end; i += 3)
            {
                func(
                    submesh.vertex_offset + data.indices[i],
                    submesh.vertex_offset + data.indices[i + 1],
                    submesh.vertex_offset + data.indices[i + 2]);
            }
        }
    }

    void GenerateNormals(MeshData& data)
    {
        std::vector<WideVector3> normal_sum(data.positions.size());
        ForEachTriangle(data, [&](std::uint32_t i0, std::uint32_t i1, std::uint32_t i2)
        {
            const auto edge1 = Difference(data.positions[i1], data.positions[i0]);
            const auto edge2 = Difference(data.positions[i2], data.positions[i0]);
            const auto face = Cross(edge1, edge2);

            normal_sum[i0] += face;
            normal_sum[i1] += face;
            normal_sum[i2] += face;
        });

        data.normals.resize(data.positions.size());
        for (std::size_t i = 0; i < data.normals.size(); ++i)
        {
            if (!NormalizeDirection(normal_sum[i], data.normals[i]))
                data.normals[i] = {.x = 0.0f, .y = 1.0f, .z = 0.0f};
        }
    }

    void CanonicalizeNormals(MeshData& data)
    {
        for (auto& normal : data.normals)
        {
            const auto wide = ToWide(normal);
            if (std::abs(std::sqrt(Dot(wide, wide)) - 1.0) > 1e-6)
                normal = NormalizeDirection(normal);
        }
    }

    void GenerateTangents(MeshData& data, bool has_source_uv)
    {
        std::vector<WideVector3> tangent_sum(data.positions.size());
        std::vector<WideVector3> bitangent_sum(data.positions.size());

        if (has_source_uv)
        {
            ForEachTriangle(data, [&](std::uint32_t i0, std::uint32_t i1, std::uint32_t i2)
            {
                const auto edge1 = Difference(data.positions[i1], data.positions[i0]);
                const auto edge2 = Difference(data.positions[i2], data.positions[i0]);
                const auto duv1x = static_cast<double>(data.uv0[i1].x) - data.uv0[i0].x;
                const auto duv1y = static_cast<double>(data.uv0[i1].y) - data.uv0[i0].y;
                const auto duv2x = static_cast<double>(data.uv0[i2].x) - data.uv0[i0].x;
                const auto duv2y = static_cast<double>(data.uv0[i2].y) - data.uv0[i0].y;

                const auto determinant = duv1x * duv2y - duv1y * duv2x;
                if (determinant == 0.0) return;

                const auto inverse = 1.0 / determinant;
                const auto tangent = (edge1 * duv2y - edge2 * duv1y) * inverse;
                const auto bitangent = (edge2 * duv1x - edge1 * duv2x) * inverse;

                tangent_sum[i0] += tangent;
                tangent_sum[i1] += tangent;
                tangent_sum[i2] += tangent;

                bitangent_sum[i0] += bitangent;
                bitangent_sum[i1] += bitangent;
                bitangent_sum[i2] += bitangent;
            });
        }

        data.tangents.resize(data.positions.size());
        for (std::size_t i = 0; i < data.tangents.size(); ++i)
        {
            const auto& normal = data.normals[i];
            const auto wide_normal = ToWide(normal);
            const auto projected = tangent_sum[i] - wide_normal * Dot(wide_normal, tangent_sum[i]);

            Vector3 tangent{};
            if (MaxComponent(projected) <= MaxComponent(tangent_sum[i]) * 1e-6 ||
                !NormalizeDirection(projected, tangent))
                tangent = StableTangent(normal);

            const float handedness = Dot(bitangent_sum[i], bitangent_sum[i]) > 0.0 &&
                                     Dot(Cross(wide_normal, ToWide(tangent)), bitangent_sum[i]) < 0.0
                                         ? -1.0f
                                         : 1.0f;
            data.tangents[i] = {
                .x = tangent.x,
                .y = tangent.y,
                .z = tangent.z,
                .w = handedness
            };
        }
    }

    void CanonicalizeTangents(MeshData& data)
    {
        for (std::size_t i = 0; i < data.tangents.size(); ++i)
        {
            auto& value = data.tangents[i];
            const auto& normal = data.normals[i];
            const auto wide_normal = ToWide(normal);
            auto wide_tangent = WideVector3{.x = value.x, .y = value.y, .z = value.z};
            const auto maximum = MaxComponent(wide_tangent);
            const auto normal_projection = Dot(wide_normal, wide_tangent);
            const auto projected = wide_tangent - wide_normal * normal_projection;

            Vector3 tangent{};
            if (MaxComponent(projected) <= maximum * 1e-6 ||
                !NormalizeDirection(projected, tangent))
                tangent = StableTangent(normal);
            else if (std::abs(std::sqrt(Dot(wide_tangent, wide_tangent)) - 1.0) <= 1e-6 &&
                std::abs(normal_projection) <= maximum * 1e-6)
                tangent = {
                    .x = value.x,
                    .y = value.y,
                    .z = value.z
                };

            value = {
                .x = tangent.x,
                .y = tangent.y,
                .z = tangent.z,
                .w = value.w < 0.0f ? -1.0f : 1.0f
            };
        }
    }

    void ComputeBounds(MeshData& data) noexcept
    {
        auto minimum = data.positions[0];
        auto maximum = data.positions[0];

        for (std::size_t i = 1; i < data.positions.size(); ++i)
        {
            const auto& position = data.positions[i];

            minimum.x = (std::min)(minimum.x, position.x);
            minimum.y = (std::min)(minimum.y, position.y);
            minimum.z = (std::min)(minimum.z, position.z);
            maximum.x = (std::max)(maximum.x, position.x);
            maximum.y = (std::max)(maximum.y, position.y);
            maximum.z = (std::max)(maximum.z, position.z);
        }

        data.bounds = {.min = minimum, .max = maximum};
    }
}

MeshDataResult Nex::SanitizeMeshData(MeshDesc& desc) noexcept
{
    auto& data = desc.data;
    
    const auto vertex_count = data.positions.size();

    if (vertex_count == 0)
        return Error(MeshDataErrorCode::EmptyPositions, MeshDataStream::Positions);
    if (vertex_count > (std::numeric_limits<std::uint32_t>::max)())
        return Error(MeshDataErrorCode::TooManyElements, MeshDataStream::Positions);
    if (data.indices.size() > (std::numeric_limits<std::uint32_t>::max)())
        return Error(MeshDataErrorCode::TooManyElements, MeshDataStream::Indices);
    if (data.submeshes.size() > (std::numeric_limits<std::uint32_t>::max)())
        return Error(MeshDataErrorCode::TooManyElements, MeshDataStream::Submeshes);

    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        if (!Finite(data.positions[i]))
            return Error(MeshDataErrorCode::NonFiniteValue, MeshDataStream::Positions, i);
    }

    const struct
    {
        MeshDataResult result;
    } size_checks[] = {
        {ValidateStreamSize(data.normals, vertex_count, MeshDataStream::Normals)},
        {ValidateStreamSize(data.tangents, vertex_count, MeshDataStream::Tangents)},
        {ValidateStreamSize(data.uv0, vertex_count, MeshDataStream::UV0)},
        {ValidateStreamSize(data.uv1, vertex_count, MeshDataStream::UV1)},
        {ValidateStreamSize(data.colors, vertex_count, MeshDataStream::Colors)}
    };

    for (const auto& check : size_checks)
    {
        if (!check.result) return check.result;
    }

    for (std::size_t i = 0; i < data.normals.size(); ++i)
    {
        if (!Finite(data.normals[i]))
            return Error(MeshDataErrorCode::NonFiniteValue, MeshDataStream::Normals, i);
        if (data.normals[i].x == 0.0f && data.normals[i].y == 0.0f && data.normals[i].z == 0.0f)
            return Error(MeshDataErrorCode::ZeroLengthAttribute, MeshDataStream::Normals, i);
    }

    for (std::size_t i = 0; i < data.tangents.size(); ++i)
    {
        const auto& value = data.tangents[i];
        if (!Finite(value))
            return Error(MeshDataErrorCode::NonFiniteValue, MeshDataStream::Tangents, i);
        if (value.x == 0.0f && value.y == 0.0f && value.z == 0.0f)
            return Error(MeshDataErrorCode::ZeroLengthAttribute, MeshDataStream::Tangents, i);
    }

    for (std::size_t i = 0; i < data.uv0.size(); ++i)
    {
        if (!Finite(data.uv0[i]))
            return Error(MeshDataErrorCode::NonFiniteValue, MeshDataStream::UV0, i);
    }

    for (std::size_t i = 0; i < data.uv1.size(); ++i)
    {
        if (!Finite(data.uv1[i]))
            return Error(MeshDataErrorCode::NonFiniteValue, MeshDataStream::UV1, i);
    }

    const auto index_count = data.indices.empty() ? vertex_count : data.indices.size();
    if (index_count > (std::numeric_limits<std::uint32_t>::max)())
        return Error(MeshDataErrorCode::TooManyElements, MeshDataStream::Indices);

    for (std::size_t i = 0; i < data.indices.size(); ++i)
    {
        if (data.indices[i] >= vertex_count)
            return Error(MeshDataErrorCode::IndexRange, MeshDataStream::Indices, i);
    }

    const auto submesh_count = data.submeshes.empty() ? std::size_t{1} : data.submeshes.size();
    for (std::size_t i = 0; i < submesh_count; ++i)
    {
        const MeshSubmesh generated{
            .topology = MeshTopology::TriangleList,
            .index_offset = 0,
            .index_count = static_cast<std::uint32_t>(index_count),
            .vertex_offset = 0
        };
        const auto& submesh = data.submeshes.empty() ? generated : data.submeshes[i];
        const auto indices_per_primitive = IndicesPerPrimitive(submesh.topology);
        if (indices_per_primitive == 0)
            return Error(MeshDataErrorCode::UnsupportedTopology, MeshDataStream::Submeshes, i,
                         static_cast<std::uint32_t>(i));
        if (submesh.index_count == 0)
            return Error(MeshDataErrorCode::EmptySubmesh, MeshDataStream::Submeshes, i,
                         static_cast<std::uint32_t>(i));
        if (submesh.index_offset > index_count ||
            submesh.index_count > index_count - submesh.index_offset)
            return Error(MeshDataErrorCode::IndexRange, MeshDataStream::Submeshes, i,
                         static_cast<std::uint32_t>(i));
        if (submesh.index_count % indices_per_primitive != 0)
            return Error(MeshDataErrorCode::InvalidPrimitiveCount, MeshDataStream::Submeshes, i,
                         static_cast<std::uint32_t>(i));
        if (submesh.vertex_offset >= vertex_count)
            return Error(MeshDataErrorCode::VertexRange, MeshDataStream::Submeshes, i,
                         static_cast<std::uint32_t>(i));

        const auto end = static_cast<std::size_t>(submesh.index_offset) + submesh.index_count;
        for (std::size_t index = submesh.index_offset; index < end; ++index)
        {
            const auto relative_index = data.indices.empty()
                                            ? static_cast<std::uint32_t>(index)
                                            : data.indices[index];

            if (relative_index >= vertex_count - submesh.vertex_offset)
                return Error(MeshDataErrorCode::VertexRange, MeshDataStream::Indices, index,
                             static_cast<std::uint32_t>(i));
        }
    }

    const bool generate_normals = data.normals.empty();
    const bool generate_tangents = data.tangents.empty();
    const bool has_source_uv = !data.uv0.empty();

    if (data.indices.empty())
    {
        data.indices.resize(vertex_count);
        for (std::size_t i = 0; i < vertex_count; ++i)
            data.indices[i] = static_cast<std::uint32_t>(i);
    }

    if (data.submeshes.empty())
    {
        data.submeshes.resize(1);
        data.submeshes[0] = {
            .topology = MeshTopology::TriangleList,
            .index_offset = 0,
            .index_count = static_cast<std::uint32_t>(data.indices.size()),
            .vertex_offset = 0
        };
    }

    if (data.uv0.empty()) data.uv0.resize(vertex_count);
    if (data.uv1.empty()) data.uv1.resize(vertex_count);

    if (data.colors.empty())
    {
        data.colors.resize(vertex_count);
        for (auto& color : data.colors)
            color = {.r = 255, .g = 255, .b = 255, .a = 255};
    }

    if (generate_normals)
        GenerateNormals(data);
    else
        CanonicalizeNormals(data);
    if (generate_tangents)
        GenerateTangents(data, has_source_uv);
    else
        CanonicalizeTangents(data);

    ComputeBounds(data);

    return {};
}

MaterialDataResult Nex::ValidateMaterialData(MaterialDesc& desc) noexcept
{
    const auto& material = desc.material;

    if (!std::isfinite(material.alpha_cutoff)) return Error(MaterialDataErrorCode::InFiniteData);

    if (material.alpha_cutoff < 0.f || material.alpha_cutoff > 1.f)
        return Error(MaterialDataErrorCode::InvalidAlphaCutoff);

    if (const auto pbr = desc.model.get<PBRMaterial>())
    {
        if (!Finite(pbr->base_color) ||
            !Finite(pbr->emissive) ||
            !std::isfinite(pbr->metallic) ||
            !std::isfinite(pbr->roughness) ||
            !std::isfinite(pbr->normal_scale) ||
            !std::isfinite(pbr->occlusion_strength))
            return Error(MaterialDataErrorCode::InFiniteData);

        if (pbr->metallic < 0.f || pbr->metallic > 1.f)
            return Error(MaterialDataErrorCode::InvalidMetallic);

        if (pbr->roughness < 0.f || pbr->roughness > 1.f)
            return Error(MaterialDataErrorCode::InvalidRoughness);

        if (pbr->normal_scale < 0.f)
            return Error(MaterialDataErrorCode::InvalidNormalScale);

        if (pbr->occlusion_strength < 0.f || pbr->occlusion_strength > 1.f)
            return Error(MaterialDataErrorCode::InvalidOcclusionStrength);
    }

    if (const auto phong = desc.model.get<PhongMaterial>())
    {
        if (!Finite(phong->diffuse) ||
            !Finite(phong->specular) ||
            !Finite(phong->emissive) ||
            !std::isfinite(phong->shininess))
            return Error(MaterialDataErrorCode::InFiniteData);

        if (phong->shininess <= 0.f)
            return Error(MaterialDataErrorCode::InvalidShininess);
    }

    return {};
}

const char* Nex::ToString(MeshDataErrorCode code) noexcept
{
    switch (code)
    {
    case MeshDataErrorCode::None: return "none";
    case MeshDataErrorCode::EmptyPositions: return "positions are empty";
    case MeshDataErrorCode::TooManyElements: return "element count exceeds uint32 range";
    case MeshDataErrorCode::NonFiniteValue: return "attribute contains a non-finite value";
    case MeshDataErrorCode::AttributeSizeMismatch: return "attribute size does not match positions";
    case MeshDataErrorCode::ZeroLengthAttribute: return "direction attribute has zero length";
    case MeshDataErrorCode::UnsupportedTopology: return "submesh topology is unsupported";
    case MeshDataErrorCode::EmptySubmesh: return "submesh has no indices";
    case MeshDataErrorCode::InvalidPrimitiveCount: return "submesh index count does not match its topology";
    case MeshDataErrorCode::IndexRange: return "index range is invalid";
    case MeshDataErrorCode::VertexRange: return "vertex range is invalid";
    }
    return "unknown mesh data error";
}

const char* Nex::ToString(MeshDataStream stream) noexcept
{
    switch (stream)
    {
    case MeshDataStream::None: return "none";
    case MeshDataStream::Positions: return "positions";
    case MeshDataStream::Normals: return "normals";
    case MeshDataStream::Tangents: return "tangents";
    case MeshDataStream::UV0: return "uv0";
    case MeshDataStream::UV1: return "uv1";
    case MeshDataStream::Colors: return "colors";
    case MeshDataStream::Indices: return "indices";
    case MeshDataStream::Submeshes: return "submeshes";
    }
    return "unknown";
}

const char* Nex::ToString(MaterialDataErrorCode code) noexcept
{
    switch (code)
    {
    case MaterialDataErrorCode::None: return "none";
    case MaterialDataErrorCode::InFiniteData: return "infinite data";
    case MaterialDataErrorCode::InvalidAlphaCutoff: return "invalid alpha cutoff";
    case MaterialDataErrorCode::InvalidMetallic: return "invalid metallic";
    case MaterialDataErrorCode::InvalidRoughness: return "invalid roughness";
    case MaterialDataErrorCode::InvalidNormalScale: return "invalid normal scale";
    case MaterialDataErrorCode::InvalidOcclusionStrength: return "invalid occlusion strength";
    case MaterialDataErrorCode::InvalidShininess: return "invalid shininess";
    }

    return "unknown material data error";
}