//
// Created by SherSpidey on 2026/6/5.
//

#ifndef NEXENGINE_MESHDATA_H
#define NEXENGINE_MESHDATA_H

#include <NexEngine/Core/Math.h>
#include <NexEngine/Core/BaseType.h>

namespace Nex
{
    enum class MeshTopology : std::uint8_t
    {
        TriangleList = 0,
        LineList,
        PointList,
        Count
    };

    struct AABB
    {
        Vector3 min;
        Vector3 max;
    };

    struct MeshSubmesh
    {
        MeshTopology topology = MeshTopology::TriangleList;

        std::uint32_t index_offset = 0;
        std::uint32_t index_count = 0;

        std::uint32_t vertex_offset = 0;
    };

    struct MeshData
    {
        Array<Vector3> positions;
        Array<Vector3> normals;
        Array<Vector4> tangents;
        Array<Vector2> uv0;
        Array<Vector2> uv1;
        Array<Color> colors;
        Array<std::uint32_t> indices;

        Array<MeshSubmesh> submeshes;
        AABB bounds;
    };
}


#endif //NEXENGINE_MESHDATA_H
