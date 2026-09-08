//
// Created by SherSpidey on 2026/5/28.
//

#ifndef NEXENGINE_MATH_H
#define NEXENGINE_MATH_H

#include <cmath>
#include <cstdint>

#include <NexEngine/Core/Hash.h>

namespace Nex
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float PI_DIV_2 = 1.57079632679489661923f;
    constexpr float PI_DIV_3 = 1.04719755119659774615f;
    constexpr float PI_TWO_DIV_3 = 2.09439510239319549230f;
    constexpr float PI_TWO = 6.28318530717958647692f;

    struct Vector2
    {
        float x;
        float y;
    };

    struct Vector3
    {
        float x;
        float y;
        float z;
    };

    struct Vector4
    {
        float x;
        float y;
        float z;
        float w;
    };

    struct Color
    {
        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;
        std::uint8_t a;
    };

    struct Matrix4
    {
        float values[16];
    };

    constexpr Matrix4 ZeroMatrix4() noexcept
    {
        return Matrix4{};
    }

    constexpr Matrix4 IdentityMatrix4() noexcept
    {
        return Matrix4{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
    }

    constexpr bool operator==(const Vector2& a, const Vector2& b) noexcept
    {
        return a.x == b.x && a.y == b.y;
    }

    constexpr bool operator==(const Vector3& a, const Vector3& b) noexcept
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    constexpr bool operator==(const Vector4& a, const Vector4& b) noexcept
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
    
    constexpr bool operator==(const Color& a, const Color& b) noexcept
    {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }

    inline bool Finite(const Vector2& value) noexcept
    {
        return std::isfinite(value.x) && std::isfinite(value.y);
    }

    inline bool Finite(const Vector3& value) noexcept
    {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    inline bool Finite(const Vector4& value) noexcept
    {
        return std::isfinite(value.x) && std::isfinite(value.y) &&
            std::isfinite(value.z) && std::isfinite(value.w);
    }

    constexpr float dot(const Vector2& a, const Vector2& b) noexcept
    {
        return a.x * b.x + a.y * b.y;
    }

    inline float length_squared(const Vector2& v) noexcept
    {
        return dot(v, v);
    }

    inline float length(const Vector2& v) noexcept
    {
        return std::sqrt(length_squared(v));
    }

    inline Vector2 normalize(const Vector2& v) noexcept
    {
        const auto sqr = length_squared(v);
        if (sqr < 1e-7f) [[unlikely]]
            return {0.0f, 0.0f};
        const auto inv = 1.0f / std::sqrt(sqr);
        return {v.x * inv, v.y * inv};
    }

    constexpr Vector2 operator+(const Vector2& a, const Vector2& b) noexcept
    {
        return {a.x + b.x, a.y + b.y};
    }

    constexpr Vector2 operator-(const Vector2& a, const Vector2& b) noexcept
    {
        return {a.x - b.x, a.y - b.y};
    }

    constexpr Vector2 operator-(const Vector2& v) noexcept
    {
        return {-v.x, -v.y};
    }

    constexpr Vector2 operator*(const Vector2& v, const float s) noexcept
    {
        return {v.x * s, v.y * s};
    }

    constexpr Vector2 operator*(const float s, const Vector2& v) noexcept
    {
        return v * s;
    }

    constexpr Vector2 operator/(const Vector2& v, const float s) noexcept
    {
        const auto inv = 1.0f / s;
        return {v.x * inv, v.y * inv};
    }

    constexpr Vector2& operator+=(Vector2& a, const Vector2& b) noexcept
    {
        a.x += b.x;
        a.y += b.y;
        return a;
    }

    constexpr Vector2& operator-=(Vector2& a, const Vector2& b) noexcept
    {
        a.x -= b.x;
        a.y -= b.y;
        return a;
    }

    constexpr Vector2& operator*=(Vector2& v, const float s) noexcept
    {
        v.x *= s;
        v.y *= s;
        return v;
    }

    constexpr Vector2& operator/=(Vector2& v, const float s) noexcept
    {
        const auto inv = 1.0f / s;
        v.x *= inv;
        v.y *= inv;
        return v;
    }

    constexpr float dot(const Vector3& a, const Vector3& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    constexpr Vector3 cross(const Vector3& a, const Vector3& b) noexcept
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    inline float length_squared(const Vector3& v) noexcept
    {
        return dot(v, v);
    }

    inline float length(const Vector3& v) noexcept
    {
        return std::sqrt(length_squared(v));
    }

    inline Vector3 normalize(const Vector3& v) noexcept
    {
        const auto sqr = length_squared(v);
        if (sqr < 1e-7f) [[unlikely]]
            return {0.0f, 0.0f, 0.0f};
        const auto inv = 1.0f / std::sqrt(sqr);
        return {v.x * inv, v.y * inv, v.z * inv};
    }

    constexpr Vector3 operator+(const Vector3& a, const Vector3& b) noexcept
    {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    constexpr Vector3 operator-(const Vector3& a, const Vector3& b) noexcept
    {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    constexpr Vector3 operator-(const Vector3& v) noexcept
    {
        return {-v.x, -v.y, -v.z};
    }

    constexpr Vector3 operator*(const Vector3& v, const float s) noexcept
    {
        return {v.x * s, v.y * s, v.z * s};
    }

    constexpr Vector3 operator*(const float s, const Vector3& v) noexcept
    {
        return v * s;
    }

    constexpr Vector3 operator/(const Vector3& v, const float s) noexcept
    {
        const auto inv = 1.0f / s;
        return {v.x * inv, v.y * inv, v.z * inv};
    }

    constexpr Vector3& operator+=(Vector3& a, const Vector3& b) noexcept
    {
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        return a;
    }

    constexpr Vector3& operator-=(Vector3& a, const Vector3& b) noexcept
    {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        return a;
    }

    constexpr Vector3& operator*=(Vector3& v, const float s) noexcept
    {
        v.x *= s;
        v.y *= s;
        v.z *= s;
        return v;
    }

    constexpr Vector3& operator/=(Vector3& v, const float s) noexcept
    {
        const auto inv = 1.0f / s;
        v.x *= inv;
        v.y *= inv;
        v.z *= inv;
        return v;
    }

    constexpr float dot(const Vector4& a, const Vector4& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    inline float length_squared(const Vector4& v) noexcept
    {
        return dot(v, v);
    }

    inline float length(const Vector4& v) noexcept
    {
        return std::sqrt(length_squared(v));
    }

    inline Vector4 normalize(const Vector4& v) noexcept
    {
        const auto sqr = length_squared(v);
        if (sqr < 1e-7f) [[unlikely]]
            return {0.0f, 0.0f, 0.0f, 0.0f};
        const auto inv = 1.0f / std::sqrt(sqr);
        return {v.x * inv, v.y * inv, v.z * inv, v.w * inv};
    }

    constexpr Vector4 operator+(const Vector4& a, const Vector4& b) noexcept
    {
        return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    }

    constexpr Vector4 operator-(const Vector4& a, const Vector4& b) noexcept
    {
        return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    }

    constexpr Vector4 operator-(const Vector4& v) noexcept
    {
        return {-v.x, -v.y, -v.z, -v.w};
    }

    constexpr Vector4 operator*(const Vector4& v, const float s) noexcept
    {
        return {v.x * s, v.y * s, v.z * s, v.w * s};
    }

    constexpr Vector4 operator*(const float s, const Vector4& v) noexcept
    {
        return v * s;
    }

    constexpr Vector4 operator/(const Vector4& v, const float s) noexcept
    {
        const auto inv = 1.0f / s;
        return {v.x * inv, v.y * inv, v.z * inv, v.w * inv};
    }

    constexpr Vector4& operator+=(Vector4& a, const Vector4& b) noexcept
    {
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        a.w += b.w;
        return a;
    }

    constexpr Vector4& operator-=(Vector4& a, const Vector4& b) noexcept
    {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        a.w -= b.w;
        return a;
    }

    constexpr Vector4& operator*=(Vector4& v, const float s) noexcept
    {
        v.x *= s;
        v.y *= s;
        v.z *= s;
        v.w *= s;
        return v;
    }

    constexpr Vector4& operator/=(Vector4& v, const float s) noexcept
    {
        const auto inv = 1.0f / s;
        v.x *= inv;
        v.y *= inv;
        v.z *= inv;
        v.w *= inv;
        return v;
    }

    constexpr Vector2 Vector2One() noexcept
    {
        return {1.0f, 1.0f};
    }

    constexpr Vector3 Vector3One() noexcept
    {
        return {1.0f, 1.0f, 1.0f};
    }

    constexpr Vector4 Vector4One() noexcept
    {
        return {1.0f, 1.0f, 1.0f, 1.0f};
    }

    template <>
    struct QuantizedPointTraits<Vector2, float>
    {
        static constexpr std::uint32_t ComponentCount = 2;

        static float Get(const Vector2& value, std::uint32_t index) noexcept
        {
            return index == 0 ? value.x : value.y;
        }
    };

    template <>
    struct QuantizedPointTraits<Vector3, float>
    {
        static constexpr std::uint32_t ComponentCount = 3;

        static float Get(const Vector3& value, std::uint32_t index) noexcept
        {
            if (index == 0) return value.x;
            if (index == 1) return value.y;

            return value.z;
        }
    };

    template <>
    struct QuantizedPointTraits<Vector4, float>
    {
        static constexpr std::uint32_t ComponentCount = 4;

        static float Get(const Vector4& value, std::uint32_t index) noexcept
        {
            if (index == 0) return value.x;
            if (index == 1) return value.y;
            if (index == 2) return value.z;

            return value.w;
        }
    };

    inline void HashAppend(StableHash64Builder& builder, const Vector2& vec) noexcept
    {
        builder.Append(vec.x);
        builder.Append(vec.y);
    }

    inline void HashAppend(StableHash64Builder& builder, const Vector3& vec) noexcept
    {
        builder.Append(vec.x);
        builder.Append(vec.y);
        builder.Append(vec.z);
    }

    inline void HashAppend(StableHash64Builder& builder, const Vector4& vec) noexcept
    {
        builder.Append(vec.x);
        builder.Append(vec.y);
        builder.Append(vec.z);
        builder.Append(vec.w);
    }
    
    inline void HashAppend(StableHash64Builder& builder, const Color& color) noexcept
    {
        builder.Append(color.r);
        builder.Append(color.g);
        builder.Append(color.b);
        builder.Append(color.a);
    }
}

namespace std
{
    template <>
    struct hash<Nex::Vector2>
    {
        std::size_t operator()(const Nex::Vector2& v) const noexcept
        {
            return Nex::ComputeHash(v.x, v.y);
        }
    };

    template <>
    struct hash<Nex::Vector3>
    {
        std::size_t operator()(const Nex::Vector3& v) const noexcept
        {
            return Nex::ComputeHash(v.x, v.y, v.z);
        }
    };

    template <>
    struct hash<Nex::Vector4>
    {
        std::size_t operator()(const Nex::Vector4& v) const noexcept
        {
            return Nex::ComputeHash(v.x, v.y, v.z, v.w);
        }
    };
}

#endif //NEXENGINE_MATH_H
