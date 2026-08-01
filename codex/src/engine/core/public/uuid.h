#pragma once

#include <engine/core/public/archive.h>

namespace codex {
    class CODEX_API UUID : public ISerializable
    {
    public:
        UUID() noexcept;
        explicit UUID(const u64 uuid) noexcept;

    public:
        [[nodiscard]] explicit constexpr operator u64() const noexcept { return uuid_; }
        [[nodiscard]] bool               operator==(const UUID& other) const noexcept { return uuid_ == other.uuid_; }

    public:
        [[nodiscard]] std::string to_string() const noexcept;
        [[nodiscard]] static UUID from_string(std::string_view str) noexcept;

        void archive(Archive& archive) override;

    private:
        u64 uuid_;
    };
} // namespace codex

namespace std {
    template <>
    struct hash<codex::UUID>
    {
        [[nodiscard]] inline std::size_t operator()(const codex::UUID& uuid) const noexcept
        { return hash<codex::u64>{}(static_cast<codex::u64>(uuid)); }
    };
} // namespace std

namespace fmt {
    template <>
    struct formatter<codex::UUID> : formatter<std::string_view>
    {
        auto format(const codex::UUID& uuid, format_context& ctx) const
        { return format_to(ctx.out(), "{}", uuid.to_string()); }
    };
} // namespace fmt
