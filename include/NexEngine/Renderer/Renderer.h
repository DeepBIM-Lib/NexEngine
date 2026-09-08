//
// Created by SherSpidey on 2026/8/17.
//

#ifndef NEXENGINE_RENDERER_H
#define NEXENGINE_RENDERER_H

#include <cstdint>

#include <NexEngine/Core/BaseType.h>
#include <NexEngine/Geometry/MeshData.h>

namespace Nex
{
    enum class RenderTopoFlag : std::uint8_t
    {
        Triangle = 1 << 0,
        Line = 1 << 1,
        Point = 1 << 2,
        All = Triangle | Line | Point
    };

    struct Renderer
    {
        RenderTopoFlag render_type = RenderTopoFlag::All;
        bool visible = true;
    };

    constexpr RenderTopoFlag operator|(RenderTopoFlag lhs, RenderTopoFlag rhs)
    {
        return static_cast<RenderTopoFlag>(raw(lhs) | raw(rhs));
    }

    constexpr RenderTopoFlag operator&(RenderTopoFlag lhs, RenderTopoFlag rhs) noexcept
    {
        return static_cast<RenderTopoFlag>(raw(lhs) & raw(rhs));
    }

    constexpr RenderTopoFlag operator~(RenderTopoFlag flag) noexcept
    {
        return static_cast<RenderTopoFlag>(~raw(flag));
    }

    constexpr RenderTopoFlag& operator|=(RenderTopoFlag& lhs, RenderTopoFlag rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr bool HasAnyFlag(RenderTopoFlag flags, RenderTopoFlag flag)
    {
        return raw(flags & flag) != 0;
    }

    constexpr RenderTopoFlag ToRenderTopoFlag(MeshTopology topology)
    {
        return static_cast<RenderTopoFlag>(1 << raw(topology));
    }
}

#endif //NEXENGINE_RENDERER_H
