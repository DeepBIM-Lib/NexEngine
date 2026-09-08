//
// Created by SherSpidey on 2026/6/25.
//

#ifndef NEXENGINE_HASH_H
#define NEXENGINE_HASH_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <type_traits>

#include <NexEngine/Core/BaseType.h>

namespace Nex
{
    using Hash64 = std::uint64_t;

    namespace Detail
    {
        inline constexpr std::size_t RuntimeHashMagic = sizeof(std::size_t) >= 8
                                                            ? 0x9e3779b97f4a7c15ull
                                                            : static_cast<std::size_t>(0x9e3779b9u);

        static constexpr Hash64 OffsetBasis = 14695981039346656037ull;
        static constexpr Hash64 Prime = 1099511628211ull;
    }

    // -------------------------------------------------------------------------
    // Runtime hash
    //
    // Used by std::hash / unordered_map inside one process.
    // The result is not guaranteed to remain stable across platforms or builds.
    // -------------------------------------------------------------------------

    template <typename T>
    std::size_t ComputeHash(const T& value)
        noexcept(noexcept(std::hash<T>{}(value)))
    {
        return std::hash<T>{}(value);
    }

    inline std::size_t ComputeHash(const String& str) noexcept
    {
        std::size_t seed = Detail::OffsetBasis;

        const auto* bytes = str.data();
        for (std::size_t i = 0; i < str.size(); ++i)
        {
            seed ^= static_cast<unsigned char>(bytes[i]);
            seed *= Detail::Prime;
        }

        return seed;
    }

    template <typename T>
    void HashCombine(std::size_t& seed, const T& value)
        noexcept(noexcept(ComputeHash(value)))
    {
        seed ^= ComputeHash(value) + Detail::RuntimeHashMagic + (seed << 6u) + (seed >> 2u);
    }

    template <typename First, typename Second, typename... Rest>
    void HashCombine(std::size_t& seed, const First& first, const Second& second, const Rest&... rest)
        noexcept(
            noexcept(HashCombine(seed, first)) &&
            noexcept(HashCombine(seed, second)) &&
            (noexcept(HashCombine(seed, rest)) && ...)
        )
    {
        HashCombine(seed, first);
        HashCombine(seed, second);
        (HashCombine(seed, rest), ...);
    }

    template <typename First, typename Second, typename... Rest>
    std::size_t ComputeHash(const First& first, const Second& second, const Rest&... rest)
        noexcept(
            noexcept(ComputeHash(first)) &&
            noexcept(ComputeHash(second)) &&
            (noexcept(ComputeHash(rest)) && ...)
        )
    {
        std::size_t seed = 0;
        HashCombine(seed, first, second, rest...);
        return seed;
    }

    // -------------------------------------------------------------------------
    // Spatial quantization hashing & equality
    //
    // Quantizes floating-point coordinates into discrete 64-bit integer grid 
    // buckets based on a configurable scale factor. Explicitly maps IEEE 754 
    // special values (NaN, +/-Inf) and clamps overflow to unique reserved keys.
    // -------------------------------------------------------------------------

    namespace Detail
    {
        template <typename Component, std::int32_t Scale>
        std::int64_t QuantizeComponent(Component value) noexcept
        {
            static_assert(std::is_floating_point_v<Component>);
            static_assert(Scale > 0);

            constexpr auto minimum = (std::numeric_limits<std::int64_t>::lowest)();
            constexpr auto maximum = (std::numeric_limits<std::int64_t>::max)();

            constexpr auto nan_key = minimum;
            constexpr auto negative_infinity_key = minimum + 1;
            constexpr auto minimum_finite_key = minimum + 2;
            constexpr auto maximum_finite_key = maximum - 1;
            constexpr auto positive_infinity_key = maximum;

            if (std::isnan(value)) return nan_key;

            if (std::isinf(value))
                return std::signbit(value) ? negative_infinity_key : positive_infinity_key;

            const auto scaled = static_cast<long double>(value) * static_cast<long double>(Scale);

            const auto quantized = std::trunc(scaled);

            constexpr auto minimum_finite = static_cast<long double>(minimum_finite_key);
            constexpr auto maximum_finite = static_cast<long double>(maximum_finite_key);

            if (quantized <= minimum_finite) return minimum_finite_key;
            if (quantized >= maximum_finite) return maximum_finite_key;

            return static_cast<std::int64_t>(quantized);
        }
    }

    // QuantizedPointTraits::ComponentCount
    // QuantizedPointTraits::Get(value, index)
    template <typename T, typename Component>
    struct QuantizedPointTraits;

    template <typename T, typename Component = float, std::int32_t Scale = 100000>
    struct QuantizeHasher
    {
        std::size_t operator()(const T& value) const noexcept
        {
            using Traits = QuantizedPointTraits<T, Component>;

            std::size_t seed = 0;

            for (std::uint32_t i = 0; i < Traits::ComponentCount; ++i)
            {
                const auto quantized =
                    Detail::QuantizeComponent<Component, Scale>(Traits::Get(value, i));

                HashCombine(seed, quantized);
            }

            return seed;
        }
    };

    template <typename T, typename Component = float, std::int32_t Scale = 100000>
    struct QuantizeEqual
    {
        bool operator()(const T& lhs, const T& rhs) const noexcept
        {
            using Traits = QuantizedPointTraits<T, Component>;

            for (std::uint32_t i = 0; i < Traits::ComponentCount; ++i)
            {
                const auto lhs_quantized =
                    Detail::QuantizeComponent<Component, Scale>(Traits::Get(lhs, i));
                const auto rhs_quantized =
                    Detail::QuantizeComponent<Component, Scale>(Traits::Get(rhs, i));

                if (lhs_quantized != rhs_quantized) return false;
            }

            return true;
        }
    };

    // -------------------------------------------------------------------------
    // Stable content hash
    //
    // Deterministic 64-bit FNV-1a with explicit little-endian serialization.
    // Intended for runtime resource deduplication and content cache keys.
    //
    // It is not cryptographic and must not be treated as a globally unique ID.
    // Hash matches must still be verified through exact equality.
    // -------------------------------------------------------------------------

    struct StableHash64Builder final
    {
        constexpr StableHash64Builder() noexcept = default;

        constexpr void Reset() noexcept
        {
            value_ = Detail::OffsetBasis;
        }

        constexpr Hash64 Finish() const noexcept
        {
            return value_;
        }

        constexpr StableHash64Builder& Append(bool value) noexcept
        {
            AppendByte(value ? 1u : 0u);
            return *this;
        }

        template <typename T, std::enable_if_t<
                      (std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>) ||
                      std::is_enum_v<T>, int> = 0>
        constexpr StableHash64Builder& Append(T value) noexcept
        {
            if constexpr (std::is_enum_v<T>)
            {
                using Underlying = std::underlying_type_t<T>;

                return Append(static_cast<Underlying>(value));
            }
            else
            {
                using Unsigned = std::make_unsigned_t<T>;

                const auto bits = static_cast<std::uintmax_t>(static_cast<Unsigned>(value));

                for (std::size_t i = 0; i < sizeof(Unsigned); ++i)
                {
                    const auto shift = static_cast<unsigned int>(i * 8u);

                    const auto byte = static_cast<std::uint8_t>((bits >> shift) & std::uintmax_t{0xffu});

                    AppendByte(byte);
                }

                return *this;
            }
        }

        StableHash64Builder& Append(float value) noexcept
        {
            static_assert(sizeof(float) == sizeof(std::uint32_t));

            // IEEE -0 compares equal to +0 and must therefore produce the same cache key
            if (value == 0.0f)
                value = 0.0f;

            std::uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));

            return Append(bits);
        }

        StableHash64Builder& Append(double value) noexcept
        {
            static_assert(sizeof(double) == sizeof(std::uint64_t));

            if (value == 0.0)
                value = 0.0;

            std::uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));

            return Append(bits);
        }

        // container sizes are always encoded as uint64_t, independent of size_t.
        constexpr StableHash64Builder& AppendSize(std::size_t size) noexcept
        {
            return Append(size);
        }

        // do not pass arbitrary structs because they may contain padding
        StableHash64Builder& AppendBytes(const void* data, std::size_t size) noexcept
        {
            if (size == 0) return *this;

            const auto bytes = static_cast<const unsigned char*>(data);

            for (std::size_t i = 0; i < size; ++i)
                AppendByte(bytes[i]);

            return *this;
        }

        // strings are length-prefixed to avoid ambiguous concatenations
        StableHash64Builder& AppendString(const char* data, std::size_t size) noexcept
        {
            AppendSize(size);
            return AppendBytes(data, size);
        }

        StableHash64Builder& AppendString(const String& string) noexcept
        {
            return AppendString(string.data(), string.size());
        }

        // prevent accidental pointer-to-bool hashing
        template <typename T>
        StableHash64Builder& Append(T*) = delete;

        StableHash64Builder& Append(std::nullptr_t) = delete;

        StableHash64Builder& Append(long double) = delete;

    private:
        constexpr void AppendByte(std::uint8_t byte) noexcept
        {
            value_ ^= static_cast<Hash64>(byte);
            value_ *= Detail::Prime;
        }

        Hash64 value_ = Detail::OffsetBasis;
    };
}

#endif // NEXENGINE_HASH_H
