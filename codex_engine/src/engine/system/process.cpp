#include "process.h"

#ifdef CX_PLATFORM_WINDOWS
#include <platform/windows/windows_process.h>
#elif defined(CX_PLATFORM_UNIX)
#include <platform/posix/posix_process.h>
#endif

namespace codex::sys {
    Process::Process(ProcessInfo info) noexcept
        : info_(std::move(info))
    {
    }

    Process::ProcessHandle Process::create(ProcessInfo info) noexcept
    {
#ifdef CX_PLATFORM_WINDOWS
        return ProcessHandle{ new NTProcess(std::move(info)), [](Process* ptr) { delete ptr; } };
#elif defined(CX_PLATFORM_UNIX)
        return ProcessHandle{ new POSIXProcess(std::move(info)), [](Process* ptr) { delete ptr; } };
#endif
        return nullptr;
    }
} // namespace codex::sys
