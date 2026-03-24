#ifndef CODEX_PLATFORM_WINDOWS_PROCESS_H
#define CODEX_PLATFORM_WINDOWS_PROCESS_H

#include "../../src/engine/system/process.h"

namespace codex::sys {
    class CODEX_API NTProcess : public Process
    {
        // So that Process::create() can create a Shared<NTProcess> with a custom deleter.
        friend class Process;

    private:
        PROCESS_INFORMATION proc_info_;
        STARTUPINFOA        start_info_;
        SECURITY_ATTRIBUTES attribs_;
        DWORD               exit_code_    = 0;
        HANDLE              stdin_read_   = nullptr;
        HANDLE              stdin_write_  = nullptr;
        HANDLE              stdout_read_  = nullptr;
        HANDLE              stdout_write_ = nullptr;
        HANDLE              stderr_read_  = nullptr;
        HANDLE              stderr_write_ = nullptr;
        std::thread         stdout_thread_;
        std::thread         stderr_thread_;
        std::atomic<bool>   running_ = false;

    private:
        NTProcess(ProcessInfo info) noexcept;
        ~NTProcess() noexcept override;

    private:
        void create_child_process();
        void dispose_handle(HANDLE& handle);

    public:
        void launch() override;
        i32  wait_for_exit() override;
        void write_line(const std::string_view msg) override;
    };
} // namespace codex::sys

#endif // CODEX_PLATFORM_WINDOWS_PROCESS_H
