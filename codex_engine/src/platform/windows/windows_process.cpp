#include "windows_process.h"

#include <engine/memory/public/memory.h>

namespace codex::sys {
    NTProcess::NTProcess(ProcessInfo info) noexcept
        : Process(std::move(info))
    {
        ZeroMemory(&proc_info_, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&start_info_, sizeof(STARTUPINFOA));

        start_info_.cb = sizeof(STARTUPINFOA);
    }

    NTProcess::~NTProcess() noexcept
    {
        // Cleanup after we're done.
        CloseHandle(proc_info_.hProcess);
        CloseHandle(proc_info_.hThread);

        // Cleanup the pipes if used any.
        dispose_handle(stdin_read_);
        dispose_handle(stdin_write_);
        dispose_handle(stdout_read_);
        dispose_handle(stdout_write_);
        dispose_handle(stderr_read_);
        dispose_handle(stderr_write_);

        // Wait for our threads to finish.
        if (stdout_thread_.joinable())
            stdout_thread_.join();
        if (stderr_thread_.joinable())
            stderr_thread_.join();
    }

    void NTProcess::create_child_process()
    {
        DWORD creation_flags = 0;

        // Set our handles.
        start_info_.hStdInput  = stdin_write_;
        start_info_.hStdOutput = stdout_write_;
        start_info_.hStdError  = stderr_read_;

        switch (info_.window_state)
        {
            using enum WindowState;

            case Normal: start_info_.wShowWindow = SW_SHOW; break;
            case Maximized: start_info_.wShowWindow = SW_MAXIMIZE; break;
            case Minimized: start_info_.wShowWindow = SW_MINIMIZE; break;
            case Hidden: start_info_.wShowWindow = SW_HIDE; break;
        }

        if (!info_.create_window)
            creation_flags |= CREATE_NO_WINDOW;
        else if (info_.separate_console)
            creation_flags |= CREATE_NEW_CONSOLE;

        // If we have any handle set then tell Windows to use our handles.
        if (start_info_.hStdInput || start_info_.hStdOutput || start_info_.hStdError)
            start_info_.dwFlags |= STARTF_USESTDHANDLES;

        // Try and create the process, If it fails then throw an exception.
        if (!CreateProcessA(nullptr,                                               // No app name (shellexecute)
                            (char*)info_.command.c_str(),                          // Command line (arguments if app)
                            nullptr,                                               // Process attribs
                            nullptr,                                               // Thread attribs
                            bool(start_info_.dwFlags & STARTF_USESTDHANDLES),     // Inherit handle?
                            creation_flags,                                        // Creation flags
                            nullptr,                                               // Envrionment (use parent's)
                            (info_.cwd) ? info_.cwd->c_str() : nullptr,           // CWD (use parent's)
                            &start_info_,                                          // Pointer to STARTUPINFOA
                            &proc_info_                                            // Pointer to PROCESS_INFORMATION
                            ))
        {
            const auto error_code = GetLastError();
            if (error_code == 2)
                throw FileNotFoundException("'{}': No such file or directory.", info_.command);
            else
                throw InvalidOperationException("Failed to start process. Native Error: {}", GetLastError());
        }

        if (start_info_.dwFlags & STARTF_USESTDHANDLES)
        {
            // Close pipes we no longer need since we're redirecting.
            dispose_handle(stdout_write_);
            dispose_handle(stdin_read_);
        }
    }

    void NTProcess::dispose_handle(HANDLE& handle)
    {
        if (handle && handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }

    void NTProcess::launch()
    {
        // If we want to just run a shell command then create a cmd instance
        // and pass contents of info_.command as an argument.
        if (info_.system_shell)
            info_.command = "cmd /c \"" + info_.command + "\"";

        // Security attributes for our anonymous pipes.
        SECURITY_ATTRIBUTES sa_attrs;
        sa_attrs.nLength              = sizeof(SECURITY_ATTRIBUTES);
        sa_attrs.bInheritHandle       = true;
        sa_attrs.lpSecurityDescriptor = nullptr;

        // If the user wants to redirect stdin, stdout and/or stderr.
        if (info_.redirect_stdin)
        {
            if (!CreatePipe(&stdin_read_, &stdin_write_, &sa_attrs, 0))
            {
                throw ProcessException("Windows: Failed to create pipe for STDIN to redirect to.");
            }
            if (!SetHandleInformation(stdin_write_, HANDLE_FLAG_INHERIT, 0))
            {
                throw ProcessException("Windows: Failed to set handle information for STDIN.");
            }
        }
        if (info_.redirect_stdout)
        {
            if (!CreatePipe(&stdout_read_, &stdout_write_, &sa_attrs, 0))
            {
                throw ProcessException("Windows: Failed to create pipe for STDOUT to redirect to.");
            }
            if (!SetHandleInformation(stdout_read_, HANDLE_FLAG_INHERIT, 0))
            {
                throw ProcessException("Windows: Failed to set handle information for STDOUT.");
            }
        }
        if (info_.redirect_stderr)
        {
            if (!CreatePipe(&stderr_read_, &stderr_write_, &sa_attrs, 0))
            {
                throw ProcessException("Windows: Failed to create pipe for STDERR to redirect to.");
            }
            if (!SetHandleInformation(stderr_read_, HANDLE_FLAG_INHERIT, 0))
            {
                throw ProcessException("Windows: Failed to set handle information for STDERR.");
            }
        }

        // Create the actual process.
        create_child_process();

        // Indicate that our process is running.
        running_ = true;

        // A non-blocking thread that will wait for the process to finish,
        // get the exit code and indicate that the process has stopped.
        // Since the thread is detached, there's a chance that the owning ProcessHandle (which is just a
        // mem::Shared<Process>) might get out of scope before our thread finishes while this thread and our process are
        // still active. To fix this we can make the thread hold a strong reference to the process handle, we do this by
        // passing a dummy variable that creates an instace of our shared pointer using NewSharedFromThis() (because
        // Process inherits from mem::SharedManagable).
        std::thread(
            [dummy = new_shared_from_this(), this]() mutable
            {
                // Create the threads responsible for redirecting stdout and stderr if
                // the user wants to redirect.
                if (on_out_data_received)
                {
                    stdout_thread_ = std::thread(
                        [this]
                        {
                            char  buffer[Process::READ_BUFFER_SIZE];
                            DWORD bytes_read;
                            while (ReadFile(stdout_read_, buffer, sizeof(buffer), &bytes_read, nullptr) &&
                                   bytes_read != 0)
                                on_out_data_received(buffer, bytes_read);
                        });
                }
                if (on_err_data_received)
                {
                    stderr_thread_ = std::thread(
                        [this]
                        {
                            char  buffer[Process::READ_BUFFER_SIZE];
                            DWORD bytes_read;
                            while (ReadFile(stderr_read_, buffer, sizeof(buffer), &bytes_read, nullptr) &&
                                   bytes_read != 0)
                                on_err_data_received(buffer, bytes_read);
                        });
                }

                // Wait for our process to finish.
                WaitForSingleObject(proc_info_.hProcess, INFINITE);

                // Try and retrieve the exit code.
                GetExitCodeProcess(proc_info_.hProcess, &exit_code_);

                // Indicate that we've stopped.
                running_ = false;

                // If the user has subscribed to the exit event.
                if (info_.on_exit)
                    info_.on_exit(exit_code_);
            })
            .detach();
    }

    i32 NTProcess::wait_for_exit()
    {
        if (proc_info_.hProcess)
        {
            while (running_)
                ;
            return exit_code_;
        }
        throw InvalidOperationException("Process hasn't been launched.");
        return -1;
    }

    void NTProcess::write_line(const std::string_view msg)
    {
        if (info_.redirect_stdin)
        {
            DWORD bytes_written;
            if (!WriteFile(stdin_write_, msg.data(), msg.size(), &bytes_written, nullptr))
                throw ProcessException("Failed to write to STDIN of a sub-process.");
        } else
            throw InvalidOperationException("Cannot write to STDIN of a process that has redirection disabled.");
    }
} // namespace codex::sys
