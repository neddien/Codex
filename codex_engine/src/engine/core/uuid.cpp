#include "public/uuid.h"

namespace codex {
    std::random_device                 UUID::s_random_device_;
    std::mt19937_64                    UUID::s_generator_(s_random_device_());
    std::uniform_int_distribution<u64> UUID::s_distribution_;

    UUID::UUID() noexcept
        : uuid_(s_distribution_(s_generator_))
    {
    }

    UUID::UUID(const u64 uuid) noexcept
        : uuid_(uuid)
    {
    }
} // namespace codex
