#pragma once

#include <sdafx.h>

#define RF_CLASS(...)
#define RF_PROPERTY(...)
#define RF_SERIALIZABLE                                                                                                \
public:                                                                                                                \
    [[nodiscard]] codex::mem::Box<codex::NativeBehaviour> Clone() const override                                       \
    {                                                                                                                  \
        return codex::mem::Box<std::decay_t<std::remove_pointer_t<decltype(this)>>>::New(*this);                       \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    void                       Serialize(ISerializationNode& node) const override;                                     \
    void                       Deserialize(const ISerializationNode& node) override;                                   \
    const codex::rf::TypeInfo& GetTypeInfo() const override;

namespace codex::rf {
    enum class PropertyType
    {
        None,
        U8,
        I8,
        U16,
        I16,
        I32,
        U32,
        I64,
        U64,
        F32,
        F64,
        F128,
        Boolean,
        String,
        Vector2,
        Vector3,
        Vector4,
        Vector2f,
        Vector3f,
        Vector4f,
        UserDefined,
    };

    struct Property
    {
        std::string  name;        // Variable name (e.g., "m_Velocity")
        PropertyType type;        // Property type
        usize        offset;      // Offset in bytes from object start
        std::string  displayName; // Display name for editor
        std::string  category;    // Category for grouping

        Property(const std::string_view name, const PropertyType type, const usize offset,
                 const std::string_view displayName = "", const std::string_view category = "General")
            : name{ name }
            , type{ type }
            , offset{ offset }
            , displayName{ displayName.empty() ? name : displayName }
            , category{ category }
        {
        }
    };

    class TypeInfo
    {
    public:
        explicit TypeInfo(const std::string_view typeName)
            : m_TypeName{ typeName }
        {
        }

    public:
        [[nodiscard]] inline std::string_view             GetTypeName() const noexcept { return m_TypeName; }
        [[nodiscard]] inline const std::vector<Property>& GetProperties() const noexcept { return m_Properties; }
        inline void AddProperty(const std::string_view name, const PropertyType type, const usize offset,
                                const std::string_view displayName = "", const std::string_view category = "General")
        {
            m_Properties.emplace_back(name, type, offset, displayName, category);
        }
        inline const Property* FindProperty(const std::string_view name) const noexcept
        {
            for (const auto& prop : m_Properties)
            {
                if (prop.name == name)
                {
                    return &prop;
                }
            }
            return nullptr;
        }

        // Helper to get property value by name
        template <typename T>
        T* GetPropertyValue(object obj, const std::string_view propertyName) const noexcept
        {
            const Property* prop = FindProperty(propertyName);
            if (!prop)
                return nullptr;

            u8* obj_bytes = reinterpret_cast<u8*>(obj);
            return reinterpret_cast<T*>(obj_bytes + prop->offset);
        }

    private:
        std::string           m_TypeName;
        std::vector<Property> m_Properties;
    };

    template <typename T>
    [[nodiscard]] constexpr PropertyType TypeOf() noexcept
    {
        using enum PropertyType;

        if constexpr (std::is_same_v<T, u8>)
            return U8;
        else if constexpr (std::is_same_v<T, i8>)
            return I8;
        else if constexpr (std::is_same_v<T, i16>)
            return I16;
        else if constexpr (std::is_same_v<T, u16>)
            return U16;
        else if constexpr (std::is_same_v<T, i32>)
            return I32;
        else if constexpr (std::is_same_v<T, u32>)
            return U32;
        else if constexpr (std::is_same_v<T, i64>)
            return I64;
        else if constexpr (std::is_same_v<T, u64>)
            return U64;
        else if constexpr (std::is_same_v<T, f32>)
            return F32;
        else if constexpr (std::is_same_v<T, f128>)
            return F128;
        else if constexpr (std::is_same_v<T, bool>)
            return Boolean;
        else if constexpr (std::is_same_v<T, codex::math::Vector3f>)
            return Vector3f;
        else if constexpr (std::is_same_v<T, codex::math::Vector2f>)
            return Vector2f;
        else if constexpr (std::is_same_v<T, codex::math::Vector3>)
            return Vector3;
        else if constexpr (std::is_same_v<T, codex::math::Vector2>)
            return Vector2;
        else if constexpr (std::is_same_v<T, std::string>)
            return String;

        static_assert("Type not supported");
        return None;
    }

    template <typename T>
    [[nodiscard]] constexpr PropertyType TypeOf(T) noexcept
    {
        return TypeOf<T>();
    }
} // namespace codex::rf
