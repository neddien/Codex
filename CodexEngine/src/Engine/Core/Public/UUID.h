#ifndef CODEX_CORE_UUID_H
#define CODEX_CORE_UUID_H

namespace codex {
    class CODEX_API UUID
    {
    private:
        u64 m_UUID;

    private:
        static std::random_device                 s_RandomDevice;
        static std::mt19937_64                    s_Generator;
        static std::uniform_int_distribution<u64> s_Distribution;

    public:
        UUID() noexcept;
        explicit UUID(const u64 uuid) noexcept;

    public:
        [[nodiscard]] explicit constexpr operator u64() const noexcept { return m_UUID; }
        [[nodiscard]] bool               operator==(const UUID& other) const noexcept { return m_UUID == other.m_UUID; }

    public:
        [[nodiscard]] inline std::string ToString() const noexcept { return std::to_string(m_UUID); }
    };
} // namespace codex

namespace std {
    template <>
    struct hash<codex::UUID>
    {
        [[nodiscard]] inline std::size_t operator()(const codex::UUID& uuid) const noexcept
        {
            return hash<codex::u64>{}(static_cast<codex::u64>(uuid));
        }
    };
} // namespace std

namespace fmt {
    template <>
    struct formatter<codex::UUID> : formatter<std::string_view>
    {
        auto format(const codex::UUID& uuid, format_context& ctx) const
        {
            return format_to(ctx.out(), "{}", static_cast<codex::u64>(uuid));
        }
    };
} // namespace fmt

/*
namespace nlohmann {
    template <>
    struct adl_serializer<codex::UUID>
    {
        static void to_json(ordered_json& j, const codex::UUID& uuid) noexcept { j = static_cast<codex::u64>(uuid); }
        static void from_json(const ordered_json& j, codex::UUID& uuid) noexcept
        {
            uuid = codex::UUID(j.get<codex::u64>());
        }
    };
} // namespace nlohmann
*/

#endif // CODEX_CORE_UUID_H
