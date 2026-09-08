//
// Created by SherSpidey on 2026/6/4.
//

#include <vector>
#include <utility>
#include <unordered_map>
#include <unordered_set>

#include <NexEngine/Geometry/Primitives/SpherifiedCube.h>

using namespace Nex;
using namespace Nex::Primitives;

namespace
{
    // ─── spherified: subdivided cube surface + geometric wireframe + points ───
    template <typename VertexHandler>
    void Spherified(
        uint32_t num_subdivision,
        MeshData& data,
        VertexHandler&& handler)
    {
        data = MeshData{};
        if (num_subdivision == 0)
            return;

        const auto num_face_vertices = (num_subdivision + 1) * (num_subdivision + 1);
        const auto num_face_triangles = num_subdivision * num_subdivision * 2;
        const auto num_face_tri_indices = num_face_triangles * 3;
        constexpr auto num_faces = 6;

        const auto total_vertices = num_face_vertices * num_faces;
        const auto total_tri_indices = num_face_tri_indices * num_faces;

        data.positions.resize(total_vertices);
        data.normals.resize(total_vertices);
        data.tangents.resize(total_vertices);
        data.uv0.resize(total_vertices);

        // Keep the cube-grid position before the shape handler transforms it. It is
        // the stable topological identity shared by Cube and QuadSphere face seams.
        std::vector<Vector3> topology_positions(total_vertices);

        const auto index_upper_bound = total_tri_indices +
            num_faces * (num_subdivision + 1) * num_subdivision * 4 + total_vertices;
        data.indices.resize(index_upper_bound);

        std::vector<Edge> edges;
        const auto edge_upper_bound = num_faces * (num_subdivision + 1) * num_subdivision * 2;
        edges.reserve(edge_upper_bound);

        static constexpr Vector3 face_normals[num_faces]{
            Vector3{.x = +1, .y = 0, .z = 0},
            Vector3{.x = -1, .y = 0, .z = 0},
            Vector3{.x = 0, .y = +1, .z = 0},
            Vector3{.x = 0, .y = -1, .z = 0},
            Vector3{.x = 0, .y = 0, .z = +1},
            Vector3{.x = 0, .y = 0, .z = -1},
        };

        static constexpr Vector3 face_tangents[num_faces]{
            Vector3{.x = 0, .y = 0, .z = +1},
            Vector3{.x = 0, .y = 0, .z = -1},
            Vector3{.x = +1, .y = 0, .z = 0},
            Vector3{.x = +1, .y = 0, .z = 0},
            Vector3{.x = -1, .y = 0, .z = 0},
            Vector3{.x = +1, .y = 0, .z = 0},
        };

        for (uint32_t face = 0; face < num_faces; ++face)
        {
            // 6 ______7______ 8
            //  |    .'|    .'|
            //  |  .'  |  .'  |
            //  |.'____|.'____|
            // 3|    .'|4   .'|5
            //  |  .'  |  .'  |
            //  |.'____|.'____|
            // 0       1      2

            const auto face_base = face * num_face_vertices;

            // vertices
            for (uint32_t y = 0; y <= num_subdivision; ++y)
            {
                for (uint32_t x = 0; x <= num_subdivision; ++x)
                {
                    Vector2 uv{
                        .x = static_cast<float>(x) / static_cast<float>(num_subdivision),
                        .y = static_cast<float>(y) / static_cast<float>(num_subdivision)
                    };
                    Vector2 xy{.x = uv.x - 0.5f, .y = 0.5f - uv.y};

                    Vector3 pos;
                    switch (face)
                    {
                    case 0: pos = Vector3{.x = +0.5f, .y = xy.y, .z = +xy.x};
                        break;
                    case 1: pos = Vector3{.x = -0.5f, .y = xy.y, .z = -xy.x};
                        break;
                    case 2: pos = Vector3{.x = xy.x, .y = +0.5f, .z = +xy.y};
                        break;
                    case 3: pos = Vector3{.x = xy.x, .y = -0.5f, .z = -xy.y};
                        break;
                    case 4: pos = Vector3{.x = -xy.x, .y = xy.y, .z = +0.5f};
                        break;
                    case 5: pos = Vector3{.x = +xy.x, .y = xy.y, .z = -0.5f};
                        break;
                    default: break;
                    }

                    auto normal = face_normals[face];

                    Vector4 tangent{
                        .x = face_tangents[face].x,
                        .y = face_tangents[face].y,
                        .z = face_tangents[face].z,
                        .w = 1.0f
                    };

                    const auto idx = face_base + y * (num_subdivision + 1) + x;
                    topology_positions[idx] = pos;

                    handler(pos, normal, uv, tangent);

                    data.positions[idx] = pos;
                    data.normals[idx] = normal;
                    data.tangents[idx] = tangent;
                    data.uv0[idx] = uv;
                }
            }

            // triangle indices
            uint32_t tri_index = face * num_face_tri_indices;
            for (uint32_t y = 0; y < num_subdivision; ++y)
            {
                for (uint32_t x = 0; x < num_subdivision; ++x)
                {
                    //  01     11
                    //   *-----*
                    //   |   .'|
                    //   | .'  |
                    //   *'----*
                    //  00     10

                    const auto v00 = face_base + y * (num_subdivision + 1) + x;
                    const auto v10 = v00 + 1;
                    const auto v01 = v00 + num_subdivision + 1;
                    const auto v11 = v01 + 1;

                    data.indices[tri_index++] = v00;
                    data.indices[tri_index++] = v10;
                    data.indices[tri_index++] = v11;
                    data.indices[tri_index++] = v00;
                    data.indices[tri_index++] = v11;
                    data.indices[tri_index++] = v01;
                }
            }

            // Collect only the geometric grid edges; the triangulation diagonal is
            // intentionally excluded from the BIM wireframe representation.
            for (uint32_t y = 0; y <= num_subdivision; ++y)
            {
                for (uint32_t x = 0; x < num_subdivision; ++x)
                {
                    edges.push_back({
                        .a = face_base + y * (num_subdivision + 1) + x,
                        .b = face_base + y * (num_subdivision + 1) + x + 1
                    });
                }
            }

            for (uint32_t y = 0; y < num_subdivision; ++y)
            {
                for (uint32_t x = 0; x <= num_subdivision; ++x)
                {
                    edges.push_back({
                        .a = face_base + y * (num_subdivision + 1) + x,
                        .b = face_base + (y + 1) * (num_subdivision + 1) + x
                    });
                }
            }
        }

        std::vector<uint32_t> position_ids(total_vertices);
        std::vector<uint32_t> point_indices;
        std::unordered_map<Vector3, uint32_t> point_lookup;
        point_indices.reserve(total_vertices);

        for (uint32_t i = 0; i < total_vertices; ++i)
        {
            const auto [it, inserted] = point_lookup.emplace(
                topology_positions[i], static_cast<uint32_t>(point_indices.size()));
            if (inserted)
                point_indices.push_back(i);
            position_ids[i] = it->second;
        }

        std::vector<Edge> unique_edges;
        unique_edges.reserve(edges.size());
        std::unordered_set<uint64_t> seen_edges;
        seen_edges.reserve(edges.size());

        for (const auto& edge : edges)
        {
            auto point_a = position_ids[edge.a];
            auto point_b = position_ids[edge.b];
            if (point_a > point_b)
                std::swap(point_a, point_b);

            const auto edge_key = static_cast<uint64_t>(point_a) << 32 | point_b;
            if (seen_edges.insert(edge_key).second)
                unique_edges.push_back(edge);
        }

        const auto wireframe_offset = total_tri_indices;
        const auto wireframe_count = static_cast<uint32_t>(unique_edges.size()) * 2;
        const auto points_offset = wireframe_offset + wireframe_count;
        const auto points_count = static_cast<uint32_t>(point_indices.size());
        data.indices.resize(points_offset + points_count);

        for (uint32_t i = 0; i < unique_edges.size(); ++i)
        {
            data.indices[wireframe_offset + i * 2] = unique_edges[i].a;
            data.indices[wireframe_offset + i * 2 + 1] = unique_edges[i].b;
        }
        for (uint32_t i = 0; i < points_count; ++i)
            data.indices[points_offset + i] = point_indices[i];

        // All representations share one vertex allocation and index buffer. Generic
        // rendering can select the matching topology/PSO from these submesh ranges.
        data.submeshes.resize(3);
        data.submeshes[0].topology = MeshTopology::TriangleList;
        data.submeshes[0].index_count = total_tri_indices;
        data.submeshes[0].vertex_offset = 0;
        data.submeshes[0].index_offset = 0;

        data.submeshes[1].topology = MeshTopology::LineList;
        data.submeshes[1].index_count = wireframe_count;
        data.submeshes[1].vertex_offset = 0;
        data.submeshes[1].index_offset = wireframe_offset;

        data.submeshes[2].topology = MeshTopology::PointList;
        data.submeshes[2].index_count = points_count;
        data.submeshes[2].vertex_offset = 0;
        data.submeshes[2].index_offset = points_offset;
    }
}

void Primitives::Generate(Cube& cube, float size)
{
    Spherified(1, cube.data,
               [size](Vector3& pos, Vector3&, Vector2&, Vector4&)
               {
                   pos *= size;
               });
}


void Primitives::Generate(QuadSphere& sphere, float radius)
{
    Spherified(8, sphere.data,
               [radius](Vector3& pos, Vector3& normal, Vector2& uv, Vector4& tangent)
               {
                   normal = normalize(pos);
                   pos = normal * radius;

                   // project pre-spherified face tangent onto the sphere tangent plane
                   const Vector3 T{.x = tangent.x, .y = tangent.y, .z = tangent.z};
                   auto tangent_dir = normalize(T - normal * dot(T, normal));

                   if (abs(dot(tangent_dir, tangent_dir)) <= 1e-7)
                       tangent_dir = cross(normal, Vector3{.x = 0, .y = 1, .z = 0});

                   if (abs(dot(tangent_dir, tangent_dir)) <= 1e-7)
                       tangent_dir = Vector3{.x = 1, .y = 0, .z = 0};

                   tangent = Vector4{.x = tangent_dir.x, .y = tangent_dir.y, .z = tangent_dir.z, .w = 1.0f};
               });
}
