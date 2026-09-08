//
// Created by SherSpidey on 2026/6/5.
//

#ifndef NEXENGINE_BASETYPE_H
#define NEXENGINE_BASETYPE_H

#include <cstdint>
#include <type_traits>
#include <utility>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <iterator>
#include <memory>

namespace Nex
{
    template <typename E, E First = static_cast<E>(0), E Last = E::Count>
    class EnumRange
    {
        using U = std::underlying_type_t<E>;

    public:
        struct Iterator
        {
            U val;

            constexpr E operator*() const
            {
                return static_cast<E>(val);
            }

            constexpr Iterator& operator++()
            {
                ++val;
                return *this;
            }

            constexpr bool operator!=(Iterator o) const
            {
                return val != o.val;
            }
        };

        constexpr Iterator begin() const { return {static_cast<U>(First)}; }
        constexpr Iterator end() const { return {static_cast<U>(Last)}; }
    };

    template <typename E, E First = static_cast<E>(0), E Last = E::Count>
    constexpr auto enum_range()
    {
        return EnumRange<E, First, Last>{};
    }

    template <typename T>
    constexpr auto raw(T e) noexcept
    {
        static_assert(std::is_enum_v<T>, "Template argument must be an enum type.");

        return static_cast<std::underlying_type_t<T>>(e);
    }

    template <typename T>
    struct ArrayView
    {
        const T* data = nullptr;
        std::size_t size = 0;

        constexpr const T* begin() const noexcept { return data; }
        constexpr const T* end() const noexcept { return data + size; }
        constexpr const T& operator[](std::size_t idx) const noexcept { return data[idx]; }
        constexpr bool empty() const noexcept { return size == 0; }
    };

    template <typename T>
    struct MutableArrayView
    {
        T* data = nullptr;
        std::size_t size = 0;

        constexpr T* begin() const noexcept { return data; }
        constexpr T* end() const noexcept { return data + size; }
        constexpr T& operator[](std::size_t idx) const noexcept { return data[idx]; }
        constexpr bool empty() const noexcept { return size == 0; }
    };

    template <typename T>
    struct Array
    {
        T* raw_data = nullptr;
        std::size_t data_size = 0;
        std::size_t data_capacity = 0;

    private:
        void reset() noexcept
        {
            if (raw_data)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    for (std::size_t i = 0; i < data_size; ++i)
                        raw_data[i].~T();
                }
                std::free(raw_data);
            }

            raw_data = nullptr;
            data_size = 0;
            data_capacity = 0;
        }

    public:
        Array() = default;

        explicit Array(std::size_t capacity)
        {
            reserve(capacity);
        }

        template <typename It>
        Array(It first, It last)
        {
            auto n = static_cast<std::size_t>(std::distance(first, last));
            if (n == 0) return;

            raw_data = static_cast<T*>(std::malloc(n * sizeof(T)));
            if (!raw_data) std::abort();
            data_capacity = n;

            std::uninitialized_copy(first, last, raw_data);

            data_size = n;
        }

        ~Array()
        {
            reset();
        }

        // move
        Array(Array&& other) noexcept
            : raw_data(other.raw_data)
              , data_size(other.data_size)
              , data_capacity(other.data_capacity)
        {
            other.raw_data = nullptr;
            other.data_size = 0;
            other.data_capacity = 0;
        }

        // move
        Array& operator=(Array&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                raw_data = other.raw_data;
                data_size = other.data_size;
                data_capacity = other.data_capacity;
                other.raw_data = nullptr;
                other.data_size = 0;
                other.data_capacity = 0;
            }
            return *this;
        }

        // no copy
        Array(const Array&) = delete;
        Array& operator=(const Array&) = delete;

        T* data() { return raw_data; }
        const T* data() const { return raw_data; }
        std::size_t size() const { return data_size; }
        std::size_t capacity() const { return data_capacity; }
        bool empty() const { return data_size == 0; }

        T& operator[](std::size_t i) { return raw_data[i]; }
        const T& operator[](std::size_t i) const { return raw_data[i]; }

        T* begin() { return raw_data; }
        T* end() { return raw_data + data_size; }
        const T* begin() const { return raw_data; }
        const T* end() const { return raw_data + data_size; }

        ArrayView<T> view() const { return {raw_data, data_size}; }
        MutableArrayView<T> view_mut() { return {raw_data, data_size}; }

        void reserve(std::size_t new_capacity)
        {
            if (new_capacity <= data_capacity)
                return;

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                auto new_data = static_cast<T*>(std::realloc(raw_data, new_capacity * sizeof(T)));
                if (!new_data) std::abort();
                raw_data = new_data;
            }
            else
            {
                auto new_data = static_cast<T*>(std::malloc(new_capacity * sizeof(T)));
                if (!new_data) std::abort();

                for (std::size_t i = 0; i < data_size; ++i)
                {
                    new(&new_data[i]) T(std::move(raw_data[i]));
                    raw_data[i].~T();
                }
                std::free(raw_data);
                raw_data = new_data;
            }
            data_capacity = new_capacity;
        }

        template <typename... Args>
        T& emplace_back(Args&&... args)
        {
            if (data_size == data_capacity)
            {
                std::size_t new_capacity = data_capacity == 0 ? 8 : data_capacity + data_capacity / 2;
                if (new_capacity <= data_capacity)
                {
                    new_capacity = data_capacity + 1;
                }
                auto new_data = static_cast<T*>(std::malloc(new_capacity * sizeof(T)));
                if (!new_data) std::abort();

                new(&new_data[data_size]) T(std::forward<Args>(args)...);

                if constexpr (std::is_trivially_copyable_v<T>)
                {
                    if (raw_data) std::memcpy(new_data, raw_data, data_size * sizeof(T));
                }
                else
                {
                    for (std::size_t i = 0; i < data_size; ++i)
                    {
                        new(&new_data[i]) T(std::move(raw_data[i]));
                        raw_data[i].~T();
                    }
                }

                std::free(raw_data);
                raw_data = new_data;
                data_capacity = new_capacity;

                return raw_data[data_size++];
            }

            new(&raw_data[data_size]) T(std::forward<Args>(args)...);
            return raw_data[data_size++];
        }

        void clear()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (std::size_t i = 0; i < data_size; ++i)
                    raw_data[i].~T();
            }
            data_size = 0;
        }

        void resize(std::size_t new_size)
        {
            if (new_size > data_capacity)
                reserve(new_size);

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (std::size_t i = new_size; i < data_size; ++i)
                    raw_data[i].~T();
            }

            for (std::size_t i = data_size; i < new_size; ++i)
                new(&raw_data[i]) T();
            data_size = new_size;
        }
    };

    using StringView = ArrayView<char>;

    struct String
    {
        char* raw_data = nullptr;
        std::size_t data_size = 0;

    private:
        void copy_from(const char* src, std::size_t len)
        {
            if (len == 0) return;
            raw_data = static_cast<char*>(std::malloc(len + 1));
            if (!raw_data) std::abort();
            std::memcpy(raw_data, src, len);
            raw_data[len] = '\0';
            data_size = len;
        }

        void reset()
        {
            if (raw_data) std::free(raw_data);
            raw_data = nullptr;
            data_size = 0;
        }

        static const char* format_scratch(StringView prefix, StringView core, StringView suffix)
        {
            constexpr std::size_t BUFFER_SIZE = 1024;
            constexpr std::size_t SLOT_COUNT = 8;

            thread_local char buffers[SLOT_COUNT][BUFFER_SIZE];
            thread_local std::size_t slot_idx = 0;

            auto buf = buffers[slot_idx];
            slot_idx = (slot_idx + 1) % SLOT_COUNT;

            auto ptr = buf;
            auto end = buf + BUFFER_SIZE - 1;

            auto append_view = [&](StringView sv)
            {
                if (!sv.data || sv.size == 0 || ptr >= end) return;

                std::size_t copy_len = (std::min)(sv.size, static_cast<std::size_t>(end - ptr));
                std::memcpy(ptr, sv.data, copy_len);
                ptr += copy_len;
            };

            append_view(prefix);
            append_view(core);
            append_view(suffix);
            *ptr = '\0';

            return buf;
        }

    public:
        String() = default;

        String(const char* str)
        {
            if (!str || *str == '\0') return;
            auto len = std::strlen(str);
            copy_from(str, len);
        }

        String(const char* str, std::size_t len)
        {
            if (!str) return;
            copy_from(str, len);
        }

        ~String()
        {
            reset();
        }

        String(String&& other) noexcept
            : raw_data(other.raw_data), data_size(other.data_size)
        {
            other.raw_data = nullptr;
            other.data_size = 0;
        }

        String& operator=(String&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                raw_data = other.raw_data;
                data_size = other.data_size;
                other.raw_data = nullptr;
                other.data_size = 0;
            }
            return *this;
        }

        String(const String& other)
        {
            copy_from(other.raw_data, other.data_size);
        }

        String& operator=(const String& other)
        {
            if (this != &other)
            {
                reset();
                copy_from(other.raw_data, other.data_size);
            }
            return *this;
        }

        const char* c_str() const { return raw_data ? raw_data : ""; }
        const char* data() const { return raw_data ? raw_data : ""; }
        std::size_t size() const { return data_size; }
        bool empty() const { return data_size == 0; }

        bool operator==(const char* other) const
        {
            if (!other) return !raw_data || *raw_data == '\0';
            if (!raw_data) return *other == '\0';
            return std::strcmp(raw_data, other) == 0;
        }

        bool operator==(const String& other) const
        {
            return *this == other.c_str();
        }

        StringView view() const { return {raw_data, data_size}; }

        const char* c_str_prefix(const char* prefix) const
        {
            if (!prefix) return c_str();
            return format_scratch({prefix, std::strlen(prefix)}, view(), {});
        }

        const char* c_str_suffix(const char* suffix) const
        {
            if (!suffix) return c_str();
            return format_scratch({}, view(), {suffix, std::strlen(suffix)});
        }

        const char* c_str_affixes(const char* prefix, const char* suffix) const
        {
            const auto pre_sv = prefix ? StringView{prefix, std::strlen(prefix)} : StringView{};
            const auto suf_sv = suffix ? StringView{suffix, std::strlen(suffix)} : StringView{};

            return format_scratch(pre_sv, view(), suf_sv);
        }
    };

    template <typename... Ts>
    struct Variant
    {
    private:
        template <typename T>
        using CleanT = std::remove_cv_t<std::remove_reference_t<T>>;

        template <std::uint32_t A, std::uint32_t B>
        struct max_of : std::integral_constant<std::uint32_t, (A > B ? A : B)>
        {
        };

        template <std::uint32_t... Vs>
        struct max_of_pack;

        template <std::uint32_t V>
        struct max_of_pack<V> : std::integral_constant<std::uint32_t, V>
        {
        };

        template <std::uint32_t V0, std::uint32_t V1, std::uint32_t... Vs>
        struct max_of_pack<V0, V1, Vs...> : max_of<V0, max_of_pack<V1, Vs...>::value>
        {
        };

        static constexpr std::uint32_t alignment = max_of_pack<alignof(Ts)...>::value;
        static constexpr std::uint32_t storage_size = max_of_pack<sizeof(Ts)...>::value;

        struct alignas(alignment) Storage
        {
            std::byte data[storage_size];
        };

        template <typename T, typename... Us>
        struct IndexOf;

        template <typename T, typename... Us>
        struct IndexOf<T, T, Us...> : std::integral_constant<std::uint32_t, 0>
        {
        };

        template <typename T, typename U, typename... Us>
        struct IndexOf<T, U, Us...> : std::integral_constant<std::uint32_t, 1 + IndexOf<T, Us...>::value>
        {
        };

        template <std::uint32_t I, typename... Us>
        struct TypeAt;

        template <typename U0, typename... Us>
        struct TypeAt<0, U0, Us...>
        {
            using type = U0;
        };

        template <std::uint32_t I, typename U0, typename... Us>
        struct TypeAt<I, U0, Us...>
        {
            // ReSharper disable once CppRedundantTypenameKeyword
            using type = typename TypeAt<I - 1, Us...>::type;
        };

        template <std::uint32_t I>
        static void destroy_impl(Storage& s) noexcept
        {
            using T = TypeAt<I, Ts...>::type;
            if constexpr (!std::is_trivially_destructible_v<T>)
                reinterpret_cast<T*>(&s)->~T();
        }

        template <std::size_t... Is>
        static void destroy_dispatch(std::uint32_t index, Storage& s, std::index_sequence<Is...>) noexcept
        {
            using DestroyFunc = void(*)(Storage&);
            static constexpr DestroyFunc table[] = {
                [](Storage& storage)
                {
                    destroy_impl<Is>(storage);
                }...
            };
            table[index](s);
        }

        template <std::size_t... Is>
        static void move_dispatch(std::uint32_t index, Storage& dst, Storage& src, std::index_sequence<Is...>) noexcept
        {
            using MoveFunc = void(*)(Storage&, Storage&);
            static constexpr MoveFunc table[] = {
                [](Storage& d, Storage& s)
                {
                    using T = TypeAt<Is, Ts...>::type;
                    new(&d) T(std::move(*reinterpret_cast<T*>(&s)));
                    if constexpr (!std::is_trivially_destructible_v<T>)
                        reinterpret_cast<T*>(&s)->~T();
                }...
            };
            table[index](dst, src);
        }

        template <std::size_t... Is>
        static void copy_dispatch(std::uint32_t index, Storage& dst, const Storage& src,
                                  std::index_sequence<Is...>) noexcept
        {
            using CopyFunc = void(*)(Storage&, const Storage&);
            static constexpr CopyFunc table[] = {
                [](Storage& d, const Storage& s)
                {
                    using T = TypeAt<Is, Ts...>::type;
                    new(&d) T(*reinterpret_cast<const T*>(&s));
                }...
            };
            table[index](dst, src);
        }

        void v_copy_to(Storage& dst) const noexcept
        {
            copy_dispatch(variant_index, dst, storage, std::make_index_sequence<sizeof...(Ts)>{});
        }

        void v_destroy() noexcept
        {
            destroy_dispatch(variant_index, storage, std::make_index_sequence<sizeof...(Ts)>{});
        }

        void v_move_to(Storage& dst) noexcept
        {
            move_dispatch(variant_index, dst, storage, std::make_index_sequence<sizeof...(Ts)>{});
        }

    public:
        std::uint32_t variant_index = 0;
        Storage storage{};

        Variant()
        {
            new(&storage) TypeAt<0, Ts...>::type();
        }

        template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Variant>>>
        explicit Variant(T&& val)
        {
            emplace<CleanT<T>>(std::forward<T>(val));
        }

        ~Variant()
        {
            v_destroy();
        }

        Variant(Variant&& other) noexcept
            : variant_index(other.variant_index)
        {
            other.v_move_to(storage);
            other.variant_index = 0;
            new(&other.storage) TypeAt<0, Ts...>::type();
        }

        Variant& operator=(Variant&& other) noexcept
        {
            if (this != &other)
            {
                v_destroy();
                variant_index = other.variant_index;
                other.v_move_to(storage);
                other.variant_index = 0;
                new(&other.storage) TypeAt<0, Ts...>::type();
            }
            return *this;
        }

        Variant(const Variant& other)
            : variant_index(other.variant_index)
        {
            other.v_copy_to(storage);
        }

        Variant& operator=(const Variant& other)
        {
            if (this != &other)
            {
                v_destroy();
                variant_index = other.variant_index;
                other.v_copy_to(storage);
            }
            return *this;
        }

        template <typename T>
        std::enable_if_t<!std::is_same_v<std::decay_t<T>, Variant>, Variant&>
        operator=(T&& val)
        {
            emplace<CleanT<T>>(std::forward<T>(val));
            return *this;
        }

        template <typename T, typename... Args>
        void emplace(Args&&... args)
        {
            v_destroy();
            new(&storage) T(std::forward<Args>(args)...);
            variant_index = IndexOf<T, Ts...>::value;
        }

        template <typename T>
        T* get()
        {
            if (variant_index != IndexOf<T, Ts...>::value)
                return nullptr;
            return reinterpret_cast<T*>(&storage);
        }

        template <typename T>
        const T* get() const
        {
            if (variant_index != IndexOf<T, Ts...>::value)
                return nullptr;
            return reinterpret_cast<const T*>(&storage);
        }

        template <typename T>
        bool is() const
        {
            return variant_index == IndexOf<T, Ts...>::value;
        }

        std::uint32_t index() const { return variant_index; }
    };
}

#endif //NEXENGINE_BASETYPE_H
