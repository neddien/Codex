#include "WindowsProcess.h"

#include <Engine/Memory/Public/Memory.h>

namespace codex::sys {
    NTProcess::NTProcess(ProcessInfo info) noexcept
        : Process(std::move(info))
    {
        ZeroMemory(&m_ProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&m_StartInfo, sizeof(STARTUPINFOA));

        m_StartInfo.cb = sizeof(STARTUPINFOA);
    }

    NTProcess::~NTProcess() noexcept
    {
        // Cleanup after we're done.
        CloseHandle(m_ProcInfo.hProcess);
        CloseHandle(m_ProcInfo.hThread);

        // Cleanup the pipes if used any.
        DisposeHandle(m_hStdInRead);
        DisposeHandle(m_hStdInWrite);
        DisposeHandle(m_hStdOutRead);
        DisposeHandle(m_hStdOutWrite);
        DisposeHandle(m_hStdErrRead);
        DisposeHandle(m_hStdErrWrite);

        // Wait for our threads to finish.
        if (m_StdOutThread.joinable())
            m_StdOutThread.join();
        if (m_StdErrThread.joinable())
            m_StdErrThread.join();
    }

    void NTProcess::CreateChildProcess()
    {
        DWORD creation_flags = 0;

        // Set our handles.
        m_StartInfo.hStdInput  = m_hStdInWrite;
        m_StartInfo.hStdOutput = m_hStdOutWrite;
        m_StartInfo.hStdError  = m_hStdErrRead;

        switch (m_Info.windowState)
        {
            using enum WindowState;

            case Normal: m_StartInfo.wShowWindow = SW_SHOW; break;
            case Maximized: m_StartInfo.wShowWindow = SW_MAXIMIZE; break;
            case Minimized: m_StartInfo.wShowWindow = SW_MINIMIZE; break;
            case Hidden: m_StartInfo.wShowWindow = SW_HIDE; break;
        }

        if (!m_Info.createWindow)
            creation_flags |= CREATE_NO_WINDOW;
        else if (m_Info.separateConsole)
            creation_flags |= CREATE_NEW_CONSOLE;

        // If we have any handle set then tell Windows to use our handles.
        if (m_StartInfo.hStdInput || m_StartInfo.hStdOutput || m_StartInfo.hStdError)
            m_StartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // Try and create the process, If it fails then throw an exception.
        if (!CreateProcessA(nullptr,                                          // No app name (shellexecute)
                            (char*)m_Info.command.c_str(),                    // Command line (arguments if app)
                            nullptr,                                          // Process attribs
                            nullptr,                                          // Thread attribs
                            bool(m_StartInfo.dwFlags & STARTF_USESTDHANDLES), // Inherit handle?
                            creation_flags,                                   // Creation flags
                            nullptr,                                          // Envrionment (use parent's)
                            (m_Info.cwd) ? m_Info.cwd->c_str() : nullptr,     // CWD (use parent's)
                            &m_StartInfo,                                     // Pointer to STARTUPINFOA
                            &m_ProcInfo                                       // Pointer to PROCESS_INFORMATION
                            ))
        {
            const auto error_code = GetLastError();
            if (error_code == 2)
                cx_throw(FileNotFoundException, "'{}': No such file or directory.", m_Info.command);
            else
                cx_throw(InvalidOperationException, "Failed to start process. Native Error: {}", GetLastError());
        }

        if (m_StartInfo.dwFlags & STARTF_USESTDHANDLES)
        {
            // Close pipes we no longer need since we're redirecting.
            DisposeHandle(m_hStdOutWrite);
            DisposeHandle(m_hStdInRead);
        }
    }

    void NTProcess::DisposeHandle(HANDLE& handle)
    {
        if (handle && handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }

    void NTProcess::Launch()
    {
        // If we want to just run a shell command then create a cmd instance
        // and pass contents of m_Info.command as an argument.
        if (m_Info.systemShell)
            m_Info.command = "cmd /c \"" + m_Info.command + "\"";

        // Security attributes for our anonymous pipes.
        SECURITY_ATTRIBUTES sa_attrs;
        sa_attrs.nLength              = sizeof(SECURITY_ATTRIBUTES);
        sa_attrs.bInheritHandle       = true;
        sa_attrs.lpSecurityDescriptor = nullptr;

        // If the user wants to redirect stdin, stdout and/or stderr.
        if (m_Info.redirectStdIn)
        {
            if (!CreatePipe(&m_hStdInRead, &m_hStdInWrite, &sa_attrs, 0))
            {
                cx_throw(ProcessException, "Windows: Failed to create pipe for STDIN to redirect to.");
            }
            if (!SetHandleInformation(m_hStdInWrite, HANDLE_FLAG_INHERIT, 0))
            {
                cx_throw(ProcessException, "Windows: Failed to set handle information for STDIN.");
            }
        }
        if (m_Info.redirectStdOut)
        {
            if (!CreatePipe(&m_hStdOutRead, &m_hStdOutWrite, &sa_attrs, 0))
            {
                cx_throw(ProcessException, "Windows: Failed to create pipe for STDOUT to redirect to.");
            }
            if (!SetHandleInformation(m_hStdOutRead, HANDLE_FLAG_INHERIT, 0))
            {
                cx_throw(ProcessException, "Windows: Failed to set handle information for STDOUT.");
            }
        }
        if (m_Info.redirectStdErr)
        {
            if (!CreatePipe(&m_hStdErrRead, &m_hStdErrWrite, &sa_attrs, 0))
            {
                cx_throw(ProcessException, "Windows: Failed to create pipe for STDERR to redirect to.");
            }
            if (!SetHandleInformation(m_hStdErrRead, HANDLE_FLAG_INHERIT, 0))
            {
                cx_throw(ProcessException, "Windows: Failed to set handle information for STDERR.");
            }
        }

        // Create the actual process.
        CreateChildProcess();

        // Indicate that our process is running.
        m_Running = true;

        // A non-blocking thread that will wait for the process to finish,
        // get the exit code and indicate that the process has stopped.
        // Since the thread is detached, there's a chance that the owning ProcessHandle (which is just a
        // mem::Shared<Process>) might get out of scope before our thread finishes while this thread and our process are
        // still active. To fix this we can make the thread hold a strong reference to the process handle, we do this by
        // passing a dummy variable that creates an instace of our shared pointer using NewSharedFromThis() (because
        // Process inherits from mem::SharedManagable).
        std::thread(
            [dummy = NewSharedFromThis(), this]() mutable
            {
                // Create the threads responsible for redirecting stdout and stderr if
                // the user wants to redirect.
                if (Event_OnOutDataReceived)
                {
                    m_StdOutThread = std::thread(
                        [this]
                        {
                            char  buffer[Process::READ_BUFFER_SIZE];
                            DWORD bytes_read;
                            while (ReadFile(m_hStdOutRead, buffer, sizeof(buffer), &bytes_read, nullptr) &&
                                   bytes_read != 0)
                                Event_OnOutDataReceived(buffer, bytes_read);
                        });
                }
                if (Event_OnErrDataReceived)
                {
                    m_StdErrThread = std::thread(
                        [this]
                        {
                            char  buffer[Process::READ_BUFFER_SIZE];
                            DWORD bytes_read;
                            while (ReadFile(m_hStdErrRead, buffer, sizeof(buffer), &bytes_read, nullptr) &&
                                   bytes_read != 0)
                                Event_OnErrDataReceived(buffer, bytes_read);
                        });
                }

                // Wait for our process to finish.
                WaitForSingleObject(m_ProcInfo.hProcess, INFINITE);

                // Try and retrieve the exit code.
                GetExitCodeProcess(m_ProcInfo.hProcess, &m_ExitCode);

                // Indicate that we've stopped.
                m_Running = false;

                // If the user has subscribed to the exit event.
                if (m_Info.onExit)
                    m_Info.onExit(m_ExitCode);
            })
            .detach();
    }

    i32 NTProcess::WaitForExit()
    {
        if (m_ProcInfo.hProcess)
        {
            while (m_Running)
                ;
            return m_ExitCode;
        }
        cx_throw(InvalidOperationException, "Process hasn't been launched.");
        return -1;
    }

    void NTProcess::WriteLine(const std::string_view msg)
    {
        if (m_Info.redirectStdIn)
        {
            DWORD bytes_written;
            if (!WriteFile(m_hStdInWrite, msg.data(), msg.size(), &bytes_written, nullptr))
                cx_throw(ProcessException, "Failed to write to STDIN of a sub-process.");
        }
        else
            cx_throw(InvalidOperationException, "Cannot write to STDIN of a process that has redirection disabled.");
    }
} // namespace codex::sys
