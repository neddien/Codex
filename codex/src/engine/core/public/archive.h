#pragma once

#include <engine/core/public/common_def.h>
#include <engine/core/public/exception.h>

#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace codex {
    CX_CUSTOM_EXCEPTION(SerializationException, "Failed to serialize.")
    CX_CUSTOM_EXCEPTION(DeserializationException, "Failed to deserialize.")

    class IArchiveBackend
    {
    public:
        virtual ~IArchiveBackend() = default;

    public:
        [[nodiscard]] virtual bool saving() const = 0;

    public:
        // `key` is honoured inside object scopes and ignored inside array scopes.
        virtual void trivial(const std::string_view key, u8& value)          = 0;
        virtual void trivial(const std::string_view key, i8& value)          = 0;
        virtual void trivial(const std::string_view key, u16& value)         = 0;
        virtual void trivial(const std::string_view key, i16& value)         = 0;
        virtual void trivial(const std::string_view key, u32& value)         = 0;
        virtual void trivial(const std::string_view key, i32& value)         = 0;
        virtual void trivial(const std::string_view key, u64& value)         = 0;
        virtual void trivial(const std::string_view key, i64& value)         = 0;
        virtual void trivial(const std::string_view key, f32& value)         = 0;
        virtual void trivial(const std::string_view key, f64& value)         = 0;
        virtual void trivial(const std::string_view key, bool& value)        = 0;
        virtual void trivial(const std::string_view key, std::string& value) = 0;

    public:
        virtual void begin_object(const std::string_view key) = 0;
        virtual void end_object()                             = 0;

        // save: `count` is the element count to emit; load: `count` is filled in.
        virtual void begin_array(const std::string_view key, usize& count) = 0;
        virtual void end_array()                                           = 0;

        // save: `count` is the entry count; load: `count` is filled in.
        virtual void begin_map(const std::string_view key, usize& count) = 0;
        // save: reads `key`; load: writes `key`. Sets the pending key for the next value.
        virtual void map_key(std::string& key) = 0;
        virtual void end_map()                 = 0;

        // Optional field. save: writes presence (returns `present_on_save`); load: returns
        // whether the field is actually present. The value (if present) is archived after.
        [[nodiscard]] virtual bool optional(const std::string_view key, const bool present_on_save) = 0;
    };

    class Archive;

    namespace detail {
        template <typename T>
        concept MemberArchive = requires(T& value, Archive& archive) { value.archive(archive); };

        template <typename T>
        concept SerdesTrivialType = std::is_same_v<T, u8> || std::is_same_v<T, i8> || std::is_same_v<T, u16> ||
                                    std::is_same_v<T, i16> || std::is_same_v<T, u32> || std::is_same_v<T, i32> ||
                                    std::is_same_v<T, u64> || std::is_same_v<T, i64> || std::is_same_v<T, f32> ||
                                    std::is_same_v<T, f64> || std::is_same_v<T, bool> || std::is_same_v<T, std::string>;

        template <typename>
        inline constexpr bool is_std_vector_v = false;
        template <typename T, typename A>
        inline constexpr bool is_std_vector_v<std::vector<T, A>> = true;

        template <typename>
        inline constexpr bool is_std_array_v = false;
        template <typename T, usize N>
        inline constexpr bool is_std_array_v<std::array<T, N>> = true;

        template <typename>
        inline constexpr bool is_std_optional_v = false;
        template <typename T>
        inline constexpr bool is_std_optional_v<std::optional<T>> = true;

        // Only string-keyed maps are supported (the document model has no other notion).
        template <typename>
        inline constexpr bool is_string_map_v = false;
        template <typename V, typename H, typename E, typename A>
        inline constexpr bool is_string_map_v<std::unordered_map<std::string, V, H, E, A>> = true;

        template <typename T>
        concept SerdesType = SerdesTrivialType<T> || is_std_vector_v<T> || is_std_array_v<T> || is_std_optional_v<T> ||
                             is_string_map_v<T>;
    } // namespace detail

    class Archive
    {
    public:
        explicit Archive(IArchiveBackend& backend, void* context = nullptr) noexcept
            : backend_{ backend }
            , context_{ context }
        {
        }

    public:
        [[nodiscard]] bool             saving() const noexcept { return backend_.saving(); }
        [[nodiscard]] bool             loading() const noexcept { return !backend_.saving(); }
        [[nodiscard]] IArchiveBackend& backend() noexcept { return backend_; }

        // Opaque user context threaded through a (de)serialization pass (e.g. an entity-id
        // remap table on scene load). Null unless the caller supplied one.
        template <typename C>
        [[nodiscard]] C* context() const noexcept
        {
            return static_cast<C*>(context_);
        }

    public:
        // Keyed field.
        template <typename T>
        Archive& operator()(const std::string_view key, T& value)
        {
            dispatch(key, value);
            return *this;
        }

        // Array element / value with no key of its own (the backend supplies the position).
        template <typename T>
        Archive& operator()(T& value)
        {
            dispatch(std::string_view{}, value);
            return *this;
        }

        // Explicitly optional field (also handled automatically for std::optional members).
        template <typename T>
        Archive& optional(const std::string_view key, T& value)
        {
            const bool present = backend_.optional(key, saving() ? true : false);
            if (present)
                dispatch(key, value);
            return *this;
        }

    private:
        template <typename T>
        void dispatch(const std::string_view key, T& value)
        {
            using U = std::remove_cvref_t<T>;
            if constexpr (detail::SerdesTrivialType<U>) {
                backend_.trivial(key, value);
            } else if constexpr (std::is_enum_v<U>) {
                archive_enum(key, value);
            } else if constexpr (detail::is_std_optional_v<U>) {
                archive_optional(key, value);
            } else if constexpr (detail::is_std_vector_v<U> || detail::is_std_array_v<U>) {
                archive_sequence(key, value);
            } else if constexpr (detail::is_string_map_v<U>) {
                archive_map(key, value);
            } else {
                backend_.begin_object(key);
                archive_value(value);
                backend_.end_object();
            }
        }

        template <typename T>
        void archive_value(T& value)
        {
            if constexpr (detail::MemberArchive<std::remove_cvref_t<T>>)
                value.archive(*this);
            else
                serialize(*this, value); // ADL: finds codex::serialize (Archive is in codex)
        }

        template <typename E>
        void archive_enum(const std::string_view key, E& value)
        {
            std::string name = saving() ? std::string{ enum_name(value) } : std::string{};
            backend_.trivial(key, name);
            if (loading()) {
                if (const auto parsed = enum_from<E>(name))
                    value = *parsed;
            }
        }

        template <typename O>
        void archive_optional(const std::string_view key, O& opt)
        {
            const bool present = backend_.optional(key, saving() ? opt.has_value() : false);
            if (present) {
                if (loading())
                    opt.emplace();
                dispatch(key, *opt);
            } else if (loading()) {
                opt.reset();
            }
        }

        template <typename C>
        void archive_sequence(const std::string_view key, C& container)
        {
            usize count = saving() ? std::size(container) : 0;
            backend_.begin_array(key, count);
            if (loading()) {
                if constexpr (detail::is_std_vector_v<std::remove_cvref_t<C>>)
                    container.resize(count);
                // std::array is fixed-size; trust the stream to match its extent.
            }
            for (auto& element : container)
                (*this)(element);
            backend_.end_array();
        }

        template <typename M>
        void archive_map(const std::string_view key, M& map)
        {
            usize count = saving() ? std::size(map) : 0;
            backend_.begin_map(key, count);
            if (saving()) {
                // Emit in sorted key order so output is deterministic (stable asset diffs and
                // byte-stable round trips), independent of the map's hash ordering.
                std::vector<std::string> keys;
                keys.reserve(map.size());
                for (const auto& [entry_key, entry_value] : map)
                    keys.push_back(entry_key);
                std::sort(keys.begin(), keys.end());
                for (auto& entry_key : keys) {
                    std::string key_copy = entry_key;
                    backend_.map_key(key_copy);
                    (*this)(map[entry_key]);
                }
            } else {
                map.clear();
                for (usize i = 0; i < count; ++i) {
                    std::string entry_key;
                    backend_.map_key(entry_key);
                    (*this)(map[entry_key]);
                }
            }
            backend_.end_map();
        }

    private:
        IArchiveBackend& backend_;
        void*            context_;
    };

    class ISerializable
    {
    public:
        virtual ~ISerializable() = default;

    public:
        // One function for both directions. Non-const because loading mutates the object.
        virtual void archive(Archive& archive) = 0;
    };

    template <typename T>
    concept Serializable = std::derived_from<T, ISerializable>;

    // Built-in serialize() overloads for common engine types using ADL (the extension seam).
    // User types can also define their own overloads in their own namespaces and Archive will find them.
    inline void serialize(Archive& ar, math::Vector2& v)
    {
        ar("x", v.x);
        ar("y", v.y);
    }
    inline void serialize(Archive& ar, math::Vector2f& v)
    {
        ar("x", v.x);
        ar("y", v.y);
    }
    inline void serialize(Archive& ar, math::Vector3& v)
    {
        ar("x", v.x);
        ar("y", v.y);
        ar("z", v.z);
    }
    inline void serialize(Archive& ar, math::Vector3f& v)
    {
        ar("x", v.x);
        ar("y", v.y);
        ar("z", v.z);
    }
    inline void serialize(Archive& ar, math::Vector4& v)
    {
        ar("x", v.x);
        ar("y", v.y);
        ar("z", v.z);
        ar("w", v.w);
    }
    inline void serialize(Archive& ar, math::Vector4f& v)
    {
        ar("x", v.x);
        ar("y", v.y);
        ar("z", v.z);
        ar("w", v.w);
    }
    inline void serialize(Archive& ar, math::Rect& v)
    {
        ar("x", v.x);
        ar("y", v.y);
        ar("w", v.w);
        ar("h", v.h);
    }
    inline void serialize(Archive& ar, math::Rectf& v)
    {
        ar("x", v.x);
        ar("y", v.y);
        ar("w", v.w);
        ar("h", v.h);
    }
} // namespace codex
