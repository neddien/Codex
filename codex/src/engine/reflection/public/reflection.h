#pragma once

#undef None

#define RF_CLASS(...)
#define RF_PROPERTY(...)
#define RF_SERIALIZABLE                                                                                                \
public:                                                                                                                \
    [[nodiscard]] codex::Box<codex::NativeBehaviour> clone() const override                                            \
    {                                                                                                                  \
        return codex::Box<std::decay_t<std::remove_pointer_t<decltype(this)>>>::make(*this);                           \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    void                       archive(codex::Archive& archive) override;                                              \
    const codex::rf::TypeInfo& type_info() const override;

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
        PrefabAsset,
        UserDefined,
    };

    // Customization point for property types that cannot be named in this header
    // (e.g. asset handles). Specialize it next to the type's own definition.
    template <typename T>
    struct type_of_ext
    {
        static constexpr PropertyType value = PropertyType::None;
    };

    struct Property
    {
        std::string  name;         // Variable name (e.g., "m_Velocity")
        PropertyType type;         // Property type
        usize        offset;       // Offset in bytes from object start
        std::string  display_name; // Display name for editor
        std::string  category;     // Category for grouping
        std::string  tooltip;      // Tooltip description
        bool         display;      // Whether to show in UI

    public:
        Property(const std::string_view name, const PropertyType type, const usize offset,
                 const std::string_view display_name = "", const std::string_view category = "General",
                 const std::string_view tooltip = "", const bool display = true)
            : name{ name }
            , type{ type }
            , offset{ offset }
            , display_name{ this->display_name.empty() ? name : display_name }
            , category{ category }
            , tooltip{ tooltip }
            , display{ display }
        {
        }
    };

    class TypeInfo
    {
    public:
        explicit TypeInfo(const std::string_view type_name)
            : type_name_{ type_name }
        {
        }

    public:
        [[nodiscard]] inline std::string_view             name() const noexcept { return type_name_; }
        [[nodiscard]] inline const std::vector<Property>& properties() const noexcept { return properties_; }
        inline void add_property(const std::string_view name, const PropertyType type, const usize offset,
                                 const std::string_view display_name = "", const std::string_view category = "General",
                                 const std::string_view tooltip = "", const bool display = true)
        {
            properties_.emplace_back(name, type, offset, display_name, category, tooltip, display);
        }
        inline const Property* find_property(const std::string_view name) const noexcept
        {
            for (const auto& prop : properties_) {
                if (prop.name == name) {
                    return &prop;
                }
            }
            return nullptr;
        }

        // Helper to get property value by name
        template <typename T>
        T* property_value(object obj, const std::string_view property_name) const noexcept
        {
            const Property* prop = find_property(property_name);
            if (!prop)
                return nullptr;

            u8* obj_bytes = reinterpret_cast<u8*>(obj);
            return reinterpret_cast<T*>(obj_bytes + prop->offset);
        }

    private:
        std::string           type_name_;
        std::vector<Property> properties_;
    };

    template <typename T>
    [[nodiscard]] constexpr PropertyType type_of() noexcept
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
        else if constexpr (std::is_same_v<T, codex::math::vec2>)
            return Vector2f;
        else if constexpr (std::is_same_v<T, codex::math::vec3>)
            return Vector3f;
        else if constexpr (std::is_same_v<T, codex::math::vec4>)
            return Vector4f;
        else if constexpr (std::is_same_v<T, codex::math::ivec2>)
            return Vector2;
        else if constexpr (std::is_same_v<T, codex::math::ivec3>)
            return Vector3;
        else if constexpr (std::is_same_v<T, codex::math::ivec4>)
            return Vector4;
        else if constexpr (std::is_same_v<T, std::string>)
            return String;
        else
            return type_of_ext<T>::value; // None unless specialized next to the type
    }

    template <typename T>
    [[nodiscard]] constexpr PropertyType type_of(T) noexcept
    {
        return type_of<T>();
    }
} // namespace codex::rf
