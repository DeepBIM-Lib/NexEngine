//
// Created by SherSpidey on 2026/5/26.
//

#ifndef NEXENGINE_HANDLE_H
#define NEXENGINE_HANDLE_H

#include <cstdint>
#include <limits>

namespace Nex
{
    inline constexpr std::uint32_t InvalidHandleIndex{(std::numeric_limits<std::uint32_t>::max)()};
    inline constexpr std::uint32_t InvalidHandleGeneration{0};

    template <typename>
    struct Handle
    {
        std::uint32_t index;
        std::uint32_t generation;
    };

    template <typename T>
    constexpr Handle<T> MakeInvalidHandle() noexcept
    {
        return Handle<T>{InvalidHandleIndex, InvalidHandleGeneration};
    }

    template <typename Tag>
    constexpr bool IsHandleValid(Handle<Tag> handle) noexcept
    {
        return handle.index != InvalidHandleIndex &&
            handle.generation != InvalidHandleGeneration;
    }

    template <typename T>
    constexpr bool operator==(const Handle<T>& lhs, const Handle<T>& rhs) noexcept
    {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }

    template <typename T>
    constexpr std::uint64_t HandleKey(Handle<T> handle) noexcept
    {
        return static_cast<std::uint64_t>(handle.generation) << 32 | handle.index;
    }
}

#endif //NEXENGINE_HANDLE_H
