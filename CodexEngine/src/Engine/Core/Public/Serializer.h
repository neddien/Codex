#pragma once

#include <sdafx.h>

#include <Engine/Reflection/Public/Reflection.h>

namespace codex {
    class ISerializationNode
    {
    public:
        using ArrayCallback = std::function<void(const ISerializationNode&)>;
        using MapCallback   = std::function<void(const std::string_view, const ISerializationNode&)>;

    public:
        virtual ~ISerializationNode() = default;

    public:
        virtual void Write(const std::string_view key, const u8 value)               = 0;
        virtual void Write(const std::string_view key, const i8 value)               = 0;
        virtual void Write(const std::string_view key, const u16 value)              = 0;
        virtual void Write(const std::string_view key, const i16 value)              = 0;
        virtual void Write(const std::string_view key, const u32 value)              = 0;
        virtual void Write(const std::string_view key, const i32 value)              = 0;
        virtual void Write(const std::string_view key, const u64 value)              = 0;
        virtual void Write(const std::string_view key, const i64 value)              = 0;
        virtual void Write(const std::string_view key, const f32 value)              = 0;
        virtual void Write(const std::string_view key, const bool value)             = 0;
        virtual void Write(const std::string_view key, const Vector2 value)          = 0;
        virtual void Write(const std::string_view key, const Vector2f value)         = 0;
        virtual void Write(const std::string_view key, const Vector3 value)          = 0;
        virtual void Write(const std::string_view key, const Vector3f value)         = 0;
        virtual void Write(const std::string_view key, const Vector4f value)         = 0;
        virtual void Write(const std::string_view key, const Vector4 value)          = 0;
        virtual void Write(const std::string_view key, const Rectf value)            = 0;
        virtual void Write(const std::string_view key, const std::string_view value) = 0;

    public:
        virtual bool Read(const std::string_view key, u8& value) const          = 0;
        virtual bool Read(const std::string_view key, i8& value) const          = 0;
        virtual bool Read(const std::string_view key, u16& value) const         = 0;
        virtual bool Read(const std::string_view key, i16& value) const         = 0;
        virtual bool Read(const std::string_view key, u32& value) const         = 0;
        virtual bool Read(const std::string_view key, i32& value) const         = 0;
        virtual bool Read(const std::string_view key, u64& value) const         = 0;
        virtual bool Read(const std::string_view key, i64& value) const         = 0;
        virtual bool Read(const std::string_view key, f32& value) const         = 0;
        virtual bool Read(const std::string_view key, bool& value) const        = 0;
        virtual bool Read(const std::string_view key, Vector2& value) const     = 0;
        virtual bool Read(const std::string_view key, Vector2f& value) const    = 0;
        virtual bool Read(const std::string_view key, Vector3& value) const     = 0;
        virtual bool Read(const std::string_view key, Vector3f& value) const    = 0;
        virtual bool Read(const std::string_view key, Vector4& value) const     = 0;
        virtual bool Read(const std::string_view key, Vector4f& value) const    = 0;
        virtual bool Read(const std::string_view key, Rectf& value) const       = 0;
        virtual bool Read(const std::string_view key, std::string& value) const = 0;

    public:
        virtual ISerializationNode&       CreateChild(const std::string_view key)       = 0;
        virtual const ISerializationNode& GetChild(const std::string_view key) const    = 0;
        virtual ISerializationNode&       BeginArray(const std::string_view key)        = 0;
        virtual void                      EndArray()                                    = 0;
        virtual ISerializationNode&       AddArrayElement()                             = 0;
        virtual ISerializationNode&       GetArray(const std::string_view key) const    = 0;
        virtual usize                     GetArraySize() const                          = 0;
        virtual const ISerializationNode& GetArrayElement(const usize idx) const        = 0;
        virtual void                      ForEachArrayElement(ArrayCallback fn) const   = 0;
        virtual ISerializationNode&       BeginMap(const std::string_view key)          = 0;
        virtual ISerializationNode&       AddMapEntry(const std::string_view key)       = 0;
        virtual void                      EndMap()                                      = 0;
        virtual ISerializationNode&       GetMap(const std::string_view key) const      = 0;
        virtual std::vector<std::string>  GetMapKeys() const                            = 0;
        virtual const ISerializationNode& GetMapEntry(const std::string_view key) const = 0;
        virtual void                      ForEachMapEntry(MapCallback callback) const   = 0;
    };

    class ISerializable
    {
    public:
        virtual ~ISerializable() = default;

    public:
        virtual void Serialize(ISerializationNode& node) const   = 0;
        virtual void Deserialize(const ISerializationNode& node) = 0;
    };
} // namespace codex
