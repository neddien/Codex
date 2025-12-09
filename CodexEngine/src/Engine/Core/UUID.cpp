#include "Public/UUID.h"

namespace codex {
    std::random_device                 UUID::s_RandomDevice;
    std::mt19937_64                    UUID::s_Generator(s_RandomDevice());
    std::uniform_int_distribution<u64> UUID::s_Distribution;

    UUID::UUID() noexcept
        : m_UUID(s_Distribution(s_Generator))
    {
    }

    UUID::UUID(const u64 uuid) noexcept
        : m_UUID(uuid)
    {
    }
} // namespace codex
