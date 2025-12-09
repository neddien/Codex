#ifndef CODEX_CORE_SERIALIZER_H
#define CODEX_CORE_SERIALIZER_H

#include <sdafx.h>

namespace codex {
    class ISerializationNode
    {
    public:
        virtual ~ISerializationNode() = default;

    public:
        virtual void Write(const std::string_view key, const u8 value)                    = 0;
        virtual void Write(const std::string_view key, const i8 value)                    = 0;
        virtual void Write(const std::string_view key, const u16 value)                   = 0;
        virtual void Write(const std::string_view key, const i16 value)                   = 0;
        virtual void Write(const std::string_view key, const u32 value)                   = 0;
        virtual void Write(const std::string_view key, const i32 value)                   = 0;
        virtual void Write(const std::string_view key, const u64 value)                   = 0;
        virtual void Write(const std::string_view key, const i64 value)                   = 0;
        virtual void Write(const std::string_view key, const f32 value)                   = 0;
        virtual void Write(const std::string_view key, const bool value)                  = 0;
        virtual void Write(const std::string_view key, const Vector2 value)               = 0;
        virtual void Write(const std::string_view key, const Vector2f value)              = 0;
        virtual void Write(const std::string_view key, const Vector3 value)               = 0;
        virtual void Write(const std::string_view key, const Vector3f value)              = 0;
        virtual void Write(const std::string_view key, const Vector4f value)              = 0;
        virtual void Write(const std::string_view key, const Rectf value)                 = 0;
        virtual void Write(const std::string_view key, const std::string_view value)      = 0;
        virtual void Write(const std::string_view key, const std::filesystem::path value) = 0;

    public:
        virtual bool Read(const std::string_view key, u8& value) const                    = 0;
        virtual bool Read(const std::string_view key, i8& value) const                    = 0;
        virtual bool Read(const std::string_view key, u16& value) const                   = 0;
        virtual bool Read(const std::string_view key, i16& value) const                   = 0;
        virtual bool Read(const std::string_view key, u32& value) const                   = 0;
        virtual bool Read(const std::string_view key, i32& value) const                   = 0;
        virtual bool Read(const std::string_view key, u64& value) const                   = 0;
        virtual bool Read(const std::string_view key, i64& value) const                   = 0;
        virtual bool Read(const std::string_view key, f32& value) const                   = 0;
        virtual bool Read(const std::string_view key, bool& value) const                  = 0;
        virtual bool Read(const std::string_view key, Vector2& value) const               = 0;
        virtual bool Read(const std::string_view key, Vector2f& value) const              = 0;
        virtual bool Read(const std::string_view key, Vector3& value) const               = 0;
        virtual bool Read(const std::string_view key, Vector3f& value) const              = 0;
        virtual bool Read(const std::string_view key, Vector4f& value) const              = 0;
        virtual bool Read(const std::string_view key, Rectf& value) const                 = 0;
        virtual bool Read(const std::string_view key, std::string& value) const           = 0;
        virtual bool Read(const std::string_view key, std::filesystem::path& value) const = 0;

    public:
        virtual ISerializationNode&       CreateChild(const std::string_view key)    = 0;
        virtual const ISerializationNode& GetChild(const std::string_view key) const = 0;
        virtual ISerializationNode&       CreateArrayElement()                       = 0;
        virtual usize                     GetArraySize() const                       = 0;
        virtual const ISerializationNode& GetArrayElement(const usize idx) const     = 0;
    };

    class ISerializable
    {
    public:
        virtual ~ISerializable() = default;

    public:
        virtual void Serialize(ISerializationNode& node) const   = 0;
        virtual void Deserialize(const ISerializationNode& node) = 0;
    };

    /*
    class CODEX_API Serializer
    {
        friend class Scene;
        friend class Entity;

    public:
        enum class Mode
        {
            Read,
            Write,
        };

    private:
        struct SerDesImpl;
        SerDesImpl* m_Impl;

    private:
        template <typename T>
        struct TypeTag
        {
        };

    public:
        void BeginObject(const std::string_view name);
        void EndObject();

    public:
        template <typename T>
        void Field(const std::string_view name, T& value)
        {
            FieldImpl(name, &value, TypeTag<T>{});
        }
        template <typename T>
        void Process(T& obj)
        {
            obj.Serialize(*this);
        }

    public:
        // static void SerializeScene(const std::filesystem::path path, const Scene& scene);
        // static void DeserializeScene(const std::filesystem::path path, Scene& scene);
    };
    */
} // namespace codex

/*
namespace nlohmann {
    template <>
    struct adl_serializer<codex::gfx::Texture2D>
    {
        static void to_json(ordered_json& j, const codex::gfx::Texture2D& texture)
        {
            const auto& props = texture.m_RawTexture->GetProperties();
            j                 = ordered_json{ { "m_Id", texture.m_Id },
                                              { "m_FilePath", texture.GetFilePath() },
                                              { "filterMode", props.filterMode },
                                              { "wrapMode", props.wrapMode },
                                              { "format", props.format } };
        }
        static void from_json(const ordered_json& j, codex::gfx::Texture2D& texture)
        {
            j.at("x").get_to(vec.x);
            j.at("y").get_to(vec.y);
        }
    };
} // namespace nlohmann
*/
#endif // CODEX_CORE_SERIALIZER_H
