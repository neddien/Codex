#include <engine/system/utils.h>

namespace codex::sys {
    u32 get_engine_thread_count() noexcept
    {
        namespace fs = std::filesystem;
        return std::distance(fs::directory_iterator("/proc/self/task"), fs::directory_iterator());
    }

    u64 get_current_thread_id() noexcept
    {
        return syscall(SYS_gettid);
    }
} // namespace codex::sys
