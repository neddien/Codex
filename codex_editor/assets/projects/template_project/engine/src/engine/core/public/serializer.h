#pragma once

#include <engine/core/public/exception.h>
#include <engine/reflection/public/reflection.h>

namespace codex {
    CX_CUSTOM_EXCEPTION(SerializationException, "Failed to serialize.");
    CX_CUSTOM_EXCEPTION(DeserializationException, "Failed to deserialize.");

    class ISerializationNode
    {
    public:
        using ArrayCallback = std::function<void(const ISerializationNode&)>;
        using MapCallback   = std::function<void(const std::string_view, const ISerializationNode&)>;

    public:
        virtual ~ISerializationNode() = default;

    public:
        virtual void write(const std::string_view key, const u8 value)               = 0;
        virtual void write(const std::string_view key, const i8 value)               = 0;
        virtual void write(const std::string_view key, const u16 value)              = 0;
        virtual void write(const std::string_view key, const i16 value)              = 0;
        virtual void write(const std::string_view key, const u32 value)              = 0;
        virtual void write(const std::string_view key, const i32 value)              = 0;
        virtual void write(const std::string_view key, const u64 value)              = 0;
        virtual void write(const std::string_view key, const i64 value)              = 0;
        virtual void write(const std::string_view key, const f32 value)              = 0;
        virtual void write(const std::string_view key, const bool value)             = 0;
        virtual void write(const std::string_view key, const Vector2 value)          = 0;
        virtual void write(const std::string_view key, const Vector2f value)         = 0;
        virtual void write(const std::string_view key, const Vector3 value)          = 0;
        virtual void write(const std::string_view key, const Vector3f value)         = 0;
        virtual void write(const std::string_view key, const Vector4f value)         = 0;
        virtual void write(const std::string_view key, const Vector4 value)          = 0;
        virtual void write(const std::string_view key, const Rectf value)            = 0;
        virtual void write(const std::string_view key, const std::string_view value) = 0;

    public:
        virtual bool read(const std::string_view key, u8& value) const          = 0;
        virtual bool read(const std::string_view key, i8& value) const          = 0;
        virtual bool read(const std::string_view key, u16& value) const         = 0;
        virtual bool read(const std::string_view key, i16& value) const         = 0;
        virtual bool read(const std::string_view key, u32& value) const         = 0;
        virtual bool read(const std::string_view key, i32& value) const         = 0;
        virtual bool read(const std::string_view key, u64& value) const         = 0;
        virtual bool read(const std::string_view key, i64& value) const         = 0;
        virtual bool read(const std::string_view key, f32& value) const         = 0;
        virtual bool read(const std::string_view key, bool& value) const        = 0;
        virtual bool read(const std::string_view key, Vector2& value) const     = 0;
        virtual bool read(const std::string_view key, Vector2f& value) const    = 0;
        virtual bool read(const std::string_view key, Vector3& value) const     = 0;
        virtual bool read(const std::string_view key, Vector3f& value) const    = 0;
        virtual bool read(const std::string_view key, Vector4& value) const     = 0;
        virtual bool read(const std::string_view key, Vector4f& value) const    = 0;
        virtual bool read(const std::string_view key, Rectf& value) const       = 0;
        virtual bool read(const std::string_view key, std::string& value) const = 0;

    public:
        virtual ISerializationNode&       create_child(const std::string_view key)       = 0;
        virtual const ISerializationNode& child(const std::string_view key) const        = 0;
        virtual ISerializationNode&       begin_array(const std::string_view key)        = 0;
        virtual void                      end_array()                                    = 0;
        virtual ISerializationNode&       add_array_element()                            = 0;
        virtual ISerializationNode&       array(const std::string_view key) const        = 0;
        virtual usize                     array_size() const                             = 0;
        virtual const ISerializationNode& array_element(const usize idx) const           = 0;
        virtual void                      for_each_array_element(ArrayCallback fn) const = 0;
        virtual ISerializationNode&       begin_map(const std::string_view key)          = 0;
        virtual ISerializationNode&       add_map_entry(const std::string_view key)      = 0;
        virtual void                      end_map()                                      = 0;
        virtual ISerializationNode&       map(const std::string_view key) const          = 0;
        virtual std::vector<std::string>  map_keys() const                               = 0;
        virtual const ISerializationNode& map_entry(const std::string_view key) const    = 0;
        virtual void                      for_each_map_entry(MapCallback callback) const = 0;
    };

    class ISerializable
    {
    public:
        virtual ~ISerializable() = default;

    public:
        virtual void serialize(ISerializationNode& node) const   = 0;
        virtual void deserialize(const ISerializationNode& node) = 0;
    };
} // namespace codex
