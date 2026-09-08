//
// Created by SherSpidey on 2026/6/4.
//

#ifndef NEXENGINE_MESHBUFFER_H
#define NEXENGINE_MESHBUFFER_H

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsTools/interface/VertexPool.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/BufferSuballocator.h>

#include <NexEngine/Core/Math.h>

namespace Nex
{
    struct PositionSlot
    {
        Vector3 position;
    };

    struct AttributeSlot
    {
        Vector3 normal;
        Vector2 uv0;
        Vector4 tangent;
    };

    struct UberSlot
    {
        Vector2 uv1;
        Vector4 color;
    };

    struct BufferPage
    {
        Diligent::RefCntAutoPtr<Diligent::IVertexPool> vertex_pool;
        Diligent::RefCntAutoPtr<Diligent::IBufferSuballocator> index_allocator;
        bool dedicated = false;
    };

    struct Allocation
    {
        std::uint32_t page_index = 0;
        Diligent::RefCntAutoPtr<Diligent::IVertexPoolAllocation> vertices;
        Diligent::RefCntAutoPtr<Diligent::IBufferSuballocation> indices;
    };
}

#endif //NEXENGINE_MESHBUFFER_H
