#include <engine/system/utils.h>

#include <tlhelp32.h>

namespace codex::sys {
    u32 get_engine_thread_count() noexcept
    {
        DWORD pid = GetCurrentProcessId();

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return -1;

        THREADENTRY32 entry;
        entry.dwSize = sizeof(entry);

        i32 count = 0;

        if (Thread32First(snapshot, &entry)) {
            do {
                if (entry.th32OwnerProcessID == pid)
                    count++;
            } while (Thread32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return count;
    }

    u64 get_current_thread_id() noexcept
    {
        return static_cast<u64>(GetCurrentThreadId());
    }
} // namespace codex::sys
