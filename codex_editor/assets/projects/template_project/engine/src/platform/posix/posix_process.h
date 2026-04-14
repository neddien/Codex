#pragma once


#include <engine/memory/public/memory.h>
#include <engine/system/process.h>

namespace codex::sys {
    class CODEX_API POSIXProcess : public Process
    {
        // So that Process::create() can create a Shared<POSIXProcess> with a custom deleter.
        friend class Process;

    private:
        pid_t             pid_         = -1;
        i32               stdin_pipe_[2]{ -1, -1 };
        i32               stdout_pipe_[2]{ -1, -1 };
        i32               stderr_pipe_[2]{ -1, -1 };
        i32               exit_code_   = -1;
        std::thread       stdout_thread_;
        std::thread       stderr_thread_;
        std::atomic<bool> running_     = false;

    public:
        POSIXProcess(ProcessInfo info) noexcept;
        ~POSIXProcess() noexcept override;

    public:
        void launch() override;
        i32  wait_for_exit() override;
        void write_line(const std::string_view msg) override;
    };
} // namespace codex::sys
