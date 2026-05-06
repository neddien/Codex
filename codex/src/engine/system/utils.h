#pragma once

namespace codex::sys {
    [[nodiscard]] u32 get_engine_thread_count() noexcept;
    [[nodiscard]] u64 get_current_thread_id() noexcept;
} // namespace codex::sys
