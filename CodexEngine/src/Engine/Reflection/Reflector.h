#ifndef CODEX_REFLECTION_REFLECTOR_H
#define CODEX_REFLECTION_REFLECTOR_H

#include <sdafx.h>

// #include <Engine/NativeBehaviour/Public/NativeBehaviour.h>

#include "Lexer.h"

#define RF_CLASS(...)
#define RF_SERIALIZABLE(...)
#define RF_INSTANCE_CREATE(dlib, name, parent)                                                                         \
    dlib->Invoke<NativeBehaviour*(const char*, codex::Entity)>("Rf_CreateInstance", name, parent)
#define RF_INSTANCE_CHECK(dlib, name) dlib->Invoke<bool(const char*)>("Rf_DoesInstanceExist", name)
#define RF_GENERATE_BODY()                                                                                             \
public:                                                                                                                \
    [[nodiscard]] codex::mem::Box<codex::NativeBehaviour> Clone() const override                                       \
    {                                                                                                                  \
        return codex::mem::Box<std::decay_t<std::remove_pointer_t<decltype(this)>>>::New(*this);                       \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    friend CODEX_EXPORT codex::NativeBehaviour* Rf_CreateInstance(const char*   className,                             \
                                                                  codex::Entity parent) noexcept;                      \
    void                                        Serialize() const noexcept override;                                   \
    codex::object                               GetField(const std::string_view fieldName) noexcept override;

// Forward declerations
namespace codex {
    class NativeBehaviour;
    class Entity;
} // namespace codex

extern "C" CODEX_EXPORT codex::NativeBehaviour* Rf_CreateInstance(const char* className, codex::Entity parent) noexcept;
extern "C" CODEX_EXPORT bool                    Rf_DoesInstanceExist(const char* className) noexcept;
extern "C" CODEX_EXPORT void                    Rf_Dummy() noexcept;

namespace codex::rf {
    // Forward declarations.
    struct RFFieldInfo;

    struct RFAttributeInfo
    {
        std::string name;
    };

    enum class RFType : u8
    {
        None,
        U8,
        I8,
        U16,
        I16,
        U32,
        I32,
        U64,
        I64,
        USize,
        Boolean,
        F32,
        F64,
        F128,
        Vector2f,
        Vector2,
        Vector3f,
        Vector3,
        Vector4f,
        Vector4,
        StdString,
        UserDefined
    };

    class RFTypeInfo
    {
    public:
        RFType                       type = RFType::None;
        std::string                  namescope;
        std::filesystem::path        file;
        std::string                  name;
        std::string                  qualifiers;
        std::vector<RFFieldInfo>     fields;
        std::vector<RFAttributeInfo> attributes;

    public:
        RFTypeInfo() noexcept = default;
        RFTypeInfo(const RFType type, const std::string_view name, const std::string_view qualifiers);
    };

    struct RFFieldInfo
    {
        std::string name;
        RFTypeInfo  type;
        std::string value;
    };

    // TODO: NativeBehaviour contain a RFScript reference instead of a JSON.
    class RFScript
    {
    private:
        std::vector<RFTypeInfo> m_Classes;
        Lexer                   m_Lexer;
        std::string             m_SourceCode;
        std::optional<Token>    m_CurrentToken  = std::nullopt;
        Token                   m_PreviousToken = Token{ .type = TokenType::Eof };
        std::filesystem::path   m_SourceFile;
        std::filesystem::path   m_OutputPath;
        std::list<std::string>  m_Scopes;

    public:
        explicit RFScript(std::filesystem::path sourceFile);

    private:
    public:
        inline std::vector<RFTypeInfo>&       GetClasses() noexcept { return m_Classes; }
        inline const std::vector<RFTypeInfo>& GetClasses() const noexcept { return m_Classes; }
        inline std::filesystem::path          GetSourceFile() const noexcept { return m_SourceFile; }

    public:
        RFScript& Parse();
        RFScript& EmitMetadata(std::filesystem::path outputPath);

    private:
        std::optional<Token>       Consume() noexcept;
        std::optional<std::string> ExpectNamespace() noexcept;
        std::optional<RFTypeInfo>  ExpectRFClass() noexcept;
        std::optional<Token>       ExpectAttribute(const std::string_view attrbName) noexcept;
        std::optional<RFFieldInfo> ExpectRFField() noexcept;
        std::string                EmitClass(const RFTypeInfo& classInfo, const bool generatedIncludes) const noexcept;

    public:
        static void EmitBaseClass(const std::filesystem::path& outputPath, const std::vector<RFScript>& files);
    };

    template <typename T>
    u8 RFTypeOf() noexcept
    {
        {
            using enum RFType;

            if constexpr (std::is_same_v<T, u8>)
                return (u8)U8;
            else if constexpr (std::is_same_v<T, i8>)
                return (u8)I8;
            else if constexpr (std::is_same_v<T, i16>)
                return (u8)I16;
            else if constexpr (std::is_same_v<T, u16>)
                return (u8)U16;
            else if constexpr (std::is_same_v<T, i32>)
                return (u8)I32;
            else if constexpr (std::is_same_v<T, u32>)
                return (u8)U32;
            else if constexpr (std::is_same_v<T, i64>)
                return (u8)I64;
            else if constexpr (std::is_same_v<T, u64>)
                return (u8)U64;
            else if constexpr (std::is_same_v<T, f32>)
                return (u8)F32;
            else if constexpr (std::is_same_v<T, f128>)
                return (u8)F128;
            else if constexpr (std::is_same_v<T, bool>)
                return (u8)Boolean;
            else if constexpr (std::is_same_v<T, codex::math::Vector3f>)
                return (u8)Vector3f;
            else if constexpr (std::is_same_v<T, codex::math::Vector2f>)
                return (u8)Vector2f;
            else if constexpr (std::is_same_v<T, codex::math::Vector3>)
                return (u8)Vector3;
            else if constexpr (std::is_same_v<T, codex::math::Vector2>)
                return (u8)Vector2;
            else if constexpr (std::is_same_v<T, std::string>)
                return (u8)StdString;
        }

        static_assert("Type not supported");
        return 0;
    }

    template <typename T>
    u8 RFTypeOf(T) noexcept
    {
        return RFTypeOf<T>();
    }
} // namespace codex::rf

#endif // CODEX_REFLECTION_REFLECTOR_H
