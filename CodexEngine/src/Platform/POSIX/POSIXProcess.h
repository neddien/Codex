#pragma once

#include <sdafx.h>

#include <Engine/Memory/Public/Memory.h>
#include <Engine/System/Process.h>

namespace codex::sys {
    class CODEX_API POSIXProcess : public Process
    {
        // So that Process::New() can create a mem::Shared<POSIXProcess> with a custom deleter.
        friend class Process;

    private:
        pid_t             m_PID = -1;
        i32               m_StdInPipe[2]{ -1, -1 };
        i32               m_StdOutPipe[2]{ -1, -1 };
        i32               m_StdErrPipe[2]{ -1, -1 };
        i32               m_ExitCode = -1;
        std::thread       m_StdOutThread;
        std::thread       m_StdErrThread;
        std::atomic<bool> m_Running = false;

    public:
        POSIXProcess(ProcessInfo info) noexcept;
        ~POSIXProcess() noexcept override;

    public:
        void Launch() override;
        i32  WaitForExit() override;
        void WriteLine(const std::string_view msg) override;
    };
} // namespace codex::sys
