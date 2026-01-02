#ifndef CODEX_PLATFORM_WINDOWS_PROCESS_H
#define CODEX_PLATFORM_WINDOWS_PROCESS_H

#include <sdafx.h>

#include "../../src/Engine/System/Process.h"

namespace codex::sys {
    class CODEX_API NTProcess : public Process
    {
        // So that Process::New() can create a Shared<NTProcess> with a custom deleter.
        friend class Process;

    private:
        PROCESS_INFORMATION m_ProcInfo;
        STARTUPINFOA        m_StartInfo;
        SECURITY_ATTRIBUTES m_Attribs;
        DWORD               m_ExitCode     = 0;
        HANDLE              m_hStdInRead   = nullptr;
        HANDLE              m_hStdInWrite  = nullptr;
        HANDLE              m_hStdOutRead  = nullptr;
        HANDLE              m_hStdOutWrite = nullptr;
        HANDLE              m_hStdErrRead  = nullptr;
        HANDLE              m_hStdErrWrite = nullptr;
        std::thread         m_StdOutThread;
        std::thread         m_StdErrThread;
        std::atomic<bool>   m_Running = false;

    private:
        NTProcess(ProcessInfo info) noexcept;
        ~NTProcess() noexcept override;

    private:
        void CreateChildProcess();
        void DisposeHandle(HANDLE& handle);

    public:
        void Launch() override;
        i32  WaitForExit() override;
        void WriteLine(const std::string_view msg) override;
    };
} // namespace codex::sys

#endif // CODEX_PLATFORM_WINDOWS_PROCESS_H