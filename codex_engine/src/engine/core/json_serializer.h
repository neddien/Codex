#pragma once

#include <engine/core/public/serializer.h>
#include <engine/memory/public/memory.h>

#include <nlohmann/json.hpp>

#define CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(fn)                                                                      \
    fn                                                                                                                 \
    {                                                                                                                  \
        json_[(key)] = (value);                                                                                        \
    }

#define CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(fn, type)                                                                 \
    fn                                                                                                                 \
    {                                                                                                                  \
        if (json_.contains((key))) {                                                                                   \
            value = json_[(key)].get<type>();                                                                          \
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
        JsonType&                                            json_;
        mutable std::vector<mem::Box<JsonSerializationNode>> children_;
        mutable bool                                         is_array_node_;
        mutable bool                                         is_map_node_;

    public:
        explicit JsonSerializationNode(JsonType& jsonObj)
            : json_(jsonObj)
            , is_array_node_(false)
        {
        }

    public:
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const u8 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const i8 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const u16 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const i16 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const u32 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const i32 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const u64 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const i64 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const f32 value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(void write(const std::string_view key, const bool value) override)
        CX_SERIALIZER_JSON_DEFAULT_WRITE_IMPL(
            void write(const std::string_view key, const std::string_view value) override)

        void write(const std::string_view key, const Vector2 value) override
        {
            json_[key]["x"] = value.x;
            json_[key]["y"] = value.y;
        }
        void write(const std::string_view key, const Vector2f value) override
        {
            json_[key]["x"] = value.x;
            json_[key]["y"] = value.y;
        }
        void write(const std::string_view key, const Vector3 value) override
        {
            json_[key]["x"] = value.x;
            json_[key]["y"] = value.y;
            json_[key]["z"] = value.z;
        }
        void write(const std::string_view key, const Vector3f value) override
        {
            json_[key]["x"] = value.x;
            json_[key]["y"] = value.y;
            json_[key]["z"] = value.z;
        }
        void write(const std::string_view key, const Vector4 value) override
        {
            json_[key]["x"] = value.x;
            json_[key]["y"] = value.y;
            json_[key]["z"] = value.z;
            json_[key]["w"] = value.w;
        }
        void write(const std::string_view key, const Vector4f value) override
        {
            json_[key]["x"] = value.x;
            json_[key]["y"] = value.y;
            json_[key]["z"] = value.z;
            json_[key]["w"] = value.w;
        }
        void write(const std::string_view key, const Rectf value) override
        {
            json_[key]["x"] = value.x;
            json_[key]["y"] = value.y;
            json_[key]["w"] = value.w;
            json_[key]["h"] = value.h;
        }

    public:
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, u8& value) const override, u8)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, i8& value) const override, i8)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, u16& value) const override, u16)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, i16& value) const override, i16)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, u32& value) const override, u32)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, i32& value) const override, i32)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, u64& value) const override, u64)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, i64& value) const override, i64)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, f32& value) const override, f32)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, bool& value) const override, bool)
        CX_SERIALIZER_JSON_DEFAULT_READ_IMPL(bool read(const std::string_view key, std::string& value) const override,
                                             std::string)

    public:
        bool read(const std::string_view key, Vector2& value) const override
        {
            if (json_.contains(key)) {
                value = Vector2{ json_[key]["x"].get<i32>(), json_[key]["y"].get<i32>() };
                return true;
            }
            return false;
        }
        bool read(const std::string_view key, Vector2f& value) const override
        {
            if (json_.contains(key)) {
                value = Vector2f{ json_[key]["x"].get<f32>(), json_[key]["y"].get<f32>() };
                return true;
            }
            return false;
        }
        bool read(const std::string_view key, Vector3& value) const override
        {
            if (json_.contains(key)) {
                value = Vector3{ json_[key]["x"].get<i32>(), json_[key]["y"].get<i32>(), json_[key]["z"].get<i32>() };
                return true;
            }
            return false;
        }
        bool read(const std::string_view key, Vector3f& value) const override
        {
            if (json_.contains(key)) {
                value = Vector3f{ json_[key]["x"].get<f32>(), json_[key]["y"].get<f32>(), json_[key]["z"].get<f32>() };
                return true;
            }
            return false;
        }
        bool read(const std::string_view key, Vector4& value) const override
        {
            if (json_.contains(key)) {
                value = Vector4{ json_[key]["x"].get<i32>(), json_[key]["y"].get<i32>(), json_[key]["w"].get<i32>(),
                                 json_[key]["h"].get<i32>() };
                return true;
            }
            return false;
        }
        bool read(const std::string_view key, Vector4f& value) const override
        {
            if (json_.contains(key)) {
                value = Vector4f{ json_[key]["x"].get<f32>(), json_[key]["y"].get<f32>(), json_[key]["z"].get<f32>(),
                                  json_[key]["w"].get<f32>() };
                return true;
            }
            return false;
        }
        bool read(const std::string_view key, Rectf& value) const override
        {
            if (json_.contains(key)) {
                value = Rectf{ json_[key]["x"].get<f32>(), json_[key]["y"].get<f32>(), json_[key]["w"].get<f32>(),
                               json_[key]["h"].get<f32>() };
                return true;
            }
            return false;
        }

    public:
        ISerializationNode& create_child(const std::string_view key) override
        {
            json_[key] = JsonType::object();
            children_.push_back(mem::Box<JsonSerializationNode>::make(json_[key]));
            return *children_.back();
        }

        const ISerializationNode& child(const std::string_view key) const override
        {
            if (json_.contains(key) && json_[key].is_object()) {
                children_.push_back(mem::Box<JsonSerializationNode>::make(const_cast<JsonType&>(json_[key])));
                return *children_.back();
            }
            throw JsonSerializerException("array_element() failed with key: {}", key);
        }

        ISerializationNode& begin_array(const std::string_view key) override
        {
            json_[key]            = nlohmann::ordered_json::array();
            auto child            = mem::Box<JsonSerializationNode>::make(json_[key]);
            child->is_array_node_ = true;
            children_.push_back(std::move(child));
            return *children_.back();
        }

        void end_array() override {}

        ISerializationNode& add_array_element() override
        {
            // This should only be called on array nodes
            assert(is_array_node_ && "add_array_element called on non-array node");

            json_.push_back(nlohmann::ordered_json::object());
            auto child = mem::Box<JsonSerializationNode>::make(json_.back());
            children_.push_back(std::move(child));
            return *children_.back();
        }

        ISerializationNode& array(const std::string_view key) const override
        {
            if (json_.contains(key) && json_[key].is_array()) {
                auto child            = mem::Box<JsonSerializationNode>::make(const_cast<JsonType&>(json_[key]));
                child->is_array_node_ = true;
                children_.push_back(std::move(child));
                return *children_.back();
            }
            throw JsonSerializerException("Array {} does not exist.", key);
        }

        size_t array_size() const override { return json_.is_array() ? json_.size() : 0; }

        const ISerializationNode& array_element(const usize idx) const override
        {
            if (json_.is_array() && idx < json_.size()) {
                children_.push_back(mem::Box<JsonSerializationNode>::make(const_cast<JsonType&>(json_[idx])));
                return *children_.back();
            }
            throw JsonSerializerException("array_element() failed with idx: {}", idx);
        }

        void for_each_array_element(ArrayCallback fn) const override
        {
            if (!json_.is_array())
                return;

            for (auto& e : json_) {
                JsonSerializationNode enode{ const_cast<JsonType&>(e) };
                fn(enode);
            }
        }

        ISerializationNode& begin_map(const std::string_view key) override
        {
            json_[key]          = nlohmann::ordered_json::object();
            auto child          = mem::Box<JsonSerializationNode>::make(json_[key]);
            child->is_map_node_ = true;
            children_.push_back(std::move(child));
            return *children_.back();
        }

        ISerializationNode& add_map_entry(const std::string_view key) override
        {
            assert(is_map_node_ && "add_map_entry called on non-map node");

            json_[key] = nlohmann::ordered_json::object();
            auto child = mem::Box<JsonSerializationNode>::make(json_[key]);
            children_.push_back(std::move(child));
            return *children_.back();
        }

        void end_map() override
        {
            // No-op for JSON
        }

        ISerializationNode& map(const std::string_view key) const override
        {
            if (json_.contains(key) && json_[key].is_object()) {
                auto child          = mem::Box<JsonSerializationNode>::make(const_cast<JsonType&>(json_[key]));
                child->is_map_node_ = true;
                children_.push_back(std::move(child));
                return *children_.back();
            }
            throw JsonSerializerException("Failed to get Map entry with Key: {}", key);
        }

        std::vector<std::string> map_keys() const override
        {
            std::vector<std::string> keys;
            if (json_.is_object()) {
                for (auto it = json_.begin(); it != json_.end(); ++it) {
                    keys.push_back(it.key());
                }
            }
            return keys;
        }

        const ISerializationNode& map_entry(const std::string_view key) const override
        {
            if (json_.contains(key)) {
                auto child = mem::Box<JsonSerializationNode>::make(const_cast<JsonType&>(json_[key]));
                children_.push_back(std::move(child));
                return *children_.back();
            }
            throw JsonSerializerException("Failed to get entry for Map: {}", key);
        }

        void for_each_map_entry(MapCallback callback) const override
        {
            if (!json_.is_object())
                return;

            for (auto it = json_.begin(); it != json_.end(); ++it) {
                JsonSerializationNode enode{ it.value() };
                callback(it.key(), enode);
            }
        }
    };
} // namespace codex
