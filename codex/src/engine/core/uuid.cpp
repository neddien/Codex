#include "public/uuid.h"

namespace codex {
    static std::random_device                 s_random_device;
    static std::mt19937_64                    s_generator{ s_random_device() };
    static std::uniform_int_distribution<u64> s_distribution;

    void UUID::archive(Archive& ar)
    {
        std::string str = ar.saving() ? to_string() : std::string{};
        ar("uuid", str);
        if (ar.loading())
            *this = from_string(str);
    }

    std::string UUID::to_string() const noexcept
    {
        char buf[19];
        std::snprintf(buf, sizeof(buf), "%04llx-%04llx-%08llx", static_cast<unsigned long long>(uuid_ >> 48) & 0xFFFF,
                      static_cast<unsigned long long>(uuid_ >> 32) & 0xFFFF,
                      static_cast<unsigned long long>(uuid_) & 0xFFFFFFFF);
        return buf;
    }

    UUID UUID::from_string(std::string_view str) noexcept
    {
        if (str.size() < 18)
            return UUID(0);

        auto parse = [](std::string_view s, int len) -> u64
        {
            u64 val = 0;
            std::from_chars(s.data(), s.data() + len, val, 16);
            return val;
        };

        const u64 a = parse(str, 4);
        const u64 b = parse(str.substr(5), 4);
        const u64 c = parse(str.substr(10), 8);
        return UUID((a << 48) | (b << 32) | c);
    }

    UUID::UUID() noexcept
        : uuid_{ s_distribution(s_generator) }
    {
    }

    UUID::UUID(const u64 uuid) noexcept
        : uuid_{ uuid }
    {
    }
} // namespace codex
