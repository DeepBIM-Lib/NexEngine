//
// Created by SherSpidey on 2026/7/10.
//

#ifndef NEXENGINE_INPUTDATAUTILS_H
#define NEXENGINE_INPUTDATAUTILS_H

#include <cstdint>

#include <NexEngine/Geometry/Mesh.h>
#include <NexEngine/Renderer/Material/Material.h>

namespace Nex
{
    enum class MeshDataErrorCode : std::uint8_t
    {
        None,
        EmptyPositions,
        TooManyElements,
        NonFiniteValue,
        AttributeSizeMismatch,
        ZeroLengthAttribute,
        UnsupportedTopology,
        EmptySubmesh,
        InvalidPrimitiveCount,
        IndexRange,
        VertexRange
    };
    
    enum class MaterialDataErrorCode : std::uint8_t
    {
        None,
        InFiniteData,
        InvalidAlphaCutoff,
        InvalidMetallic,
        InvalidRoughness,
        InvalidNormalScale,
        InvalidOcclusionStrength,
        InvalidShininess
    };

    enum class MeshDataStream : std::uint8_t
    {
        None,
        Positions,
        Normals,
        Tangents,
        UV0,
        UV1,
        Colors,
        Indices,
        Submeshes
    };

    struct MeshDataResult
    {
        MeshDataErrorCode code = MeshDataErrorCode::None;
        MeshDataStream stream = MeshDataStream::None;
        std::size_t element_index = 0;
        std::uint32_t submesh_index = 0;

        explicit operator bool() const noexcept
        {
            return code == MeshDataErrorCode::None;
        }
    };
    
    struct MaterialDataResult
    {
        MaterialDataErrorCode code = MaterialDataErrorCode::None;
        
        explicit operator bool() const noexcept
        {
            return code == MaterialDataErrorCode::None;
        }
    };

    MeshDataResult SanitizeMeshData(MeshDesc& desc) noexcept;
    MaterialDataResult ValidateMaterialData(MaterialDesc& desc) noexcept;
    
    const char* ToString(MeshDataErrorCode code) noexcept;
    const char* ToString(MeshDataStream stream) noexcept;
    const char* ToString(MaterialDataErrorCode code) noexcept;
}

#endif //NEXENGINE_INPUTDATAUTILS_H
