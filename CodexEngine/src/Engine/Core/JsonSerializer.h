#pragma once

#include <Engine/Core/Public/Serializer.h>
#include <Engine/Memory/Public/Memory.h>

#include <nlohmann/json.hpp>

#define CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(fn)                                                                      \
    fn                                                                                                                 \
    {                                                                                                                  \
        m_Json[(key)] = (value);                                                                                       \
    }

#define CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(fn, type)                                                                 \
    fn                                                                                                                 \
    {                                                                                                                  \
        if (m_Json.contains((key)))                                                                                    \
        {                                                                                                              \
            value = m_Json[(key)].get<type>();                                                                         \
            return true;                                                                                               \
        }                                                                                                              \
        return false;                                                                                                  \
    }

namespace codex {
    CX_CUSTOM_EXCEPTION(JsonSerializerException, "JSON serialization backend failed")

    class JsonSerializationNode : public ISerializationNode
    {
        using JsonType = nlohmann::ordered_json;

    private:
        JsonType&                                            m_Json;
        mutable std::vector<mem::Box<JsonSerializationNode>> m_Children;
        mutable bool                                         m_IsArrayNode;
        mutable bool                                         m_IsMapNode;

    public:
        explicit JsonSerializationNode(JsonType& jsonObj)
            : m_Json(jsonObj)
            , m_IsArrayNode(false)
        {
        }

    public:
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const u8 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const i8 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const u16 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const i16 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const u32 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const i32 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const u64 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const i64 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const f32 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void Write(const std::string_view key, const bool value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(
            void Write(const std::string_view key, const std::string_view value) override)

        void Write(const std::string_view key, const Vector2 value) override
        {
            m_Json[key]["x"] = value.x;
            m_Json[key]["y"] = value.y;
        }
        void Write(const std::string_view key, const Vector2f value) override
        {
            m_Json[key]["x"] = value.x;
            m_Json[key]["y"] = value.y;
        }
        void Write(const std::string_view key, const Vector3 value) override
        {
            m_Json[key]["x"] = value.x;
            m_Json[key]["y"] = value.y;
            m_Json[key]["z"] = value.z;
        }
        void Write(const std::string_view key, const Vector3f value) override
        {
            m_Json[key]["x"] = value.x;
            m_Json[key]["y"] = value.y;
            m_Json[key]["z"] = value.z;
        }
        void Write(const std::string_view key, const Vector4 value) override
        {
            m_Json[key]["x"] = value.x;
            m_Json[key]["y"] = value.y;
            m_Json[key]["z"] = value.z;
            m_Json[key]["w"] = value.w;
        }
        void Write(const std::string_view key, const Vector4f value) override
        {
            m_Json[key]["x"] = value.x;
            m_Json[key]["y"] = value.y;
            m_Json[key]["z"] = value.z;
            m_Json[key]["w"] = value.w;
        }
        void Write(const std::string_view key, const Rectf value) override
        {
            m_Json[key]["x"] = value.x;
            m_Json[key]["y"] = value.y;
            m_Json[key]["w"] = value.w;
            m_Json[key]["h"] = value.h;
        }

    public:
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, u8& value) const override, u8)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, i8& value) const override, i8)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, u16& value) const override, u16)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, i16& value) const override, i16)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, u32& value) const override, u32)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, i32& value) const override, i32)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, u64& value) const override, u64)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, i64& value) const override, i64)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, f32& value) const override, f32)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, bool& value) const override, bool)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool Read(const std::string_view key, std::string& value) const override,
                                             std::string)

    public:
        bool Read(const std::string_view key, Vector2& value) const override
        {
            if (m_Json.contains(key))
            {
                value = Vector2{ m_Json[key]["x"].get<i32>(), m_Json[key]["y"].get<i32>() };
                return true;
            }
            return false;
        }
        bool Read(const std::string_view key, Vector2f& value) const override
        {
            if (m_Json.contains(key))
            {
                value = Vector2f{ m_Json[key]["x"].get<f32>(), m_Json[key]["y"].get<f32>() };
                return true;
            }
            return false;
        }
        bool Read(const std::string_view key, Vector3& value) const override
        {
            if (m_Json.contains(key))
            {
                value =
                    Vector3{ m_Json[key]["x"].get<i32>(), m_Json[key]["y"].get<i32>(), m_Json[key]["z"].get<i32>() };
                return true;
            }
            return false;
        }
        bool Read(const std::string_view key, Vector3f& value) const override
        {
            if (m_Json.contains(key))
            {
                value =
                    Vector3f{ m_Json[key]["x"].get<f32>(), m_Json[key]["y"].get<f32>(), m_Json[key]["z"].get<f32>() };
                return true;
            }
            return false;
        }
        bool Read(const std::string_view key, Vector4& value) const override
        {
            if (m_Json.contains(key))
            {
                value = Vector4{ m_Json[key]["x"].get<i32>(), m_Json[key]["y"].get<i32>(), m_Json[key]["w"].get<i32>(),
                                 m_Json[key]["h"].get<i32>() };
                return true;
            }
            return false;
        }
        bool Read(const std::string_view key, Vector4f& value) const override
        {
            if (m_Json.contains(key))
            {
                value = Vector4f{ m_Json[key]["x"].get<f32>(), m_Json[key]["y"].get<f32>(), m_Json[key]["z"].get<f32>(),
                                  m_Json[key]["w"].get<f32>() };
                return true;
            }
            return false;
        }
        bool Read(const std::string_view key, Rectf& value) const override
        {
            if (m_Json.contains(key))
            {
                value = Rectf{ m_Json[key]["x"].get<f32>(), m_Json[key]["y"].get<f32>(), m_Json[key]["w"].get<f32>(),
                               m_Json[key]["h"].get<f32>() };
                return true;
            }
            return false;
        }

    public:
        ISerializationNode& CreateChild(const std::string_view key) override
        {
            m_Json[key] = JsonType::object();
            m_Children.push_back(mem::Box<JsonSerializationNode>::New(m_Json[key]));
            return *m_Children.back();
        }

        const ISerializationNode& GetChild(const std::string_view key) const override
        {
            if (m_Json.contains(key) && m_Json[key].is_object())
            {
                m_Children.push_back(mem::Box<JsonSerializationNode>::New(const_cast<JsonType&>(m_Json[key])));
                return *m_Children.back();
            }
            cx_throw(JsonSerializerException, "GetArrayElement() failed with key: {}", key);
        }

        ISerializationNode& BeginArray(const std::string_view key) override
        {
            m_Json[key]          = nlohmann::ordered_json::array();
            auto child           = mem::Box<JsonSerializationNode>::New(m_Json[key]);
            child->m_IsArrayNode = true;
            m_Children.push_back(std::move(child));
            return *m_Children.back();
        }

        void EndArray() override {}

        ISerializationNode& AddArrayElement() override
        {
            // This should only be called on array nodes
            assert(m_IsArrayNode && "AddArrayElement called on non-array node");

            m_Json.push_back(nlohmann::ordered_json::object());
            auto child = mem::Box<JsonSerializationNode>::New(m_Json.back());
            m_Children.push_back(std::move(child));
            return *m_Children.back();
        }

        ISerializationNode& GetArray(const std::string_view key) const override
        {
            if (m_Json.contains(key) && m_Json[key].is_array())
            {
                auto child           = mem::Box<JsonSerializationNode>::New(const_cast<JsonType&>(m_Json[key]));
                child->m_IsArrayNode = true;
                m_Children.push_back(std::move(child));
                return *m_Children.back();
            }
            cx_throw(JsonSerializerException, "Array {} does not exist.", key);
        }

        size_t GetArraySize() const override { return m_Json.is_array() ? m_Json.size() : 0; }

        const ISerializationNode& GetArrayElement(const usize idx) const override
        {
            if (m_Json.is_array() && idx < m_Json.size())
            {
                m_Children.push_back(mem::Box<JsonSerializationNode>::New(const_cast<JsonType&>(m_Json[idx])));
                return *m_Children.back();
            }
            cx_throw(JsonSerializerException, "GetArrayElement() failed with idx: {}", idx);
        }

        void ForEachArrayElement(ArrayCallback fn) const override
        {
            if (!m_Json.is_array())
                return;

            for (auto& e : m_Json)
            {
                JsonSerializationNode enode{ const_cast<JsonType&>(e) };
                fn(enode);
            }
        }

        ISerializationNode& BeginMap(const std::string_view key) override
        {
            m_Json[key]        = nlohmann::ordered_json::object();
            auto child         = mem::Box<JsonSerializationNode>::New(m_Json[key]);
            child->m_IsMapNode = true;
            m_Children.push_back(std::move(child));
            return *m_Children.back();
        }

        ISerializationNode& AddMapEntry(const std::string_view key) override
        {
            assert(m_IsMapNode && "AddMapEntry called on non-map node");

            m_Json[key] = nlohmann::ordered_json::object();
            auto child  = mem::Box<JsonSerializationNode>::New(m_Json[key]);
            m_Children.push_back(std::move(child));
            return *m_Children.back();
        }

        void EndMap() override
        {
            // No-op for JSON
        }

        ISerializationNode& GetMap(const std::string_view key) const override
        {
            if (m_Json.contains(key) && m_Json[key].is_object())
            {
                auto child         = mem::Box<JsonSerializationNode>::New(const_cast<JsonType&>(m_Json[key]));
                child->m_IsMapNode = true;
                m_Children.push_back(std::move(child));
                return *m_Children.back();
            }
            cx_throw(JsonSerializerException, "Failed to get Map entry with Key: {}", key);
        }

        std::vector<std::string> GetMapKeys() const override
        {
            std::vector<std::string> keys;
            if (m_Json.is_object())
            {
                for (auto it = m_Json.begin(); it != m_Json.end(); ++it)
                {
                    keys.push_back(it.key());
                }
            }
            return keys;
        }

        const ISerializationNode& GetMapEntry(const std::string_view key) const override
        {
            if (m_Json.contains(key))
            {
                auto child = mem::Box<JsonSerializationNode>::New(const_cast<JsonType&>(m_Json[key]));
                m_Children.push_back(std::move(child));
                return *m_Children.back();
            }
            cx_throw(JsonSerializerException, "Failed to get entry for Map: {}", key);
        }

        void ForEachMapEntry(MapCallback callback) const override
        {
            if (!m_Json.is_object())
                return;

            for (auto it = m_Json.begin(); it != m_Json.end(); ++it)
            {
                JsonSerializationNode enode{ it.value() };
                callback(it.key(), enode);
            }
        }
    };
} // namespace codex
