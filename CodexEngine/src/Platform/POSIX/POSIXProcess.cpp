#include "POSIXProcess.h"

namespace codex::sys {
    POSIXProcess::POSIXProcess(ProcessInfo info) noexcept
        : Process(std::move(info))
    {
    }

    POSIXProcess::~POSIXProcess() noexcept
    {
        // Close any pipe fds the parent still holds open.
        if (m_StdOutPipe[0] != -1)
            close(m_StdOutPipe[0]);
        if (m_StdErrPipe[0] != -1)
            close(m_StdErrPipe[0]);
        if (m_StdInPipe[1] != -1)
            close(m_StdInPipe[1]);

        // Wait for reader threads to finish.
        if (m_StdOutThread.joinable())
            m_StdOutThread.join();
        if (m_StdErrThread.joinable())
            m_StdErrThread.join();
    }

    void POSIXProcess::Launch()
    {
        // Create pipes BEFORE fork so both parent and child inherit the fds.
        if (m_Info.redirectStdOut)
        {
            if (pipe(m_StdOutPipe) < 0)
                cx_throw(ProcessException, "Failed to create stdout pipe: {}", std::strerror(errno));
        }
        if (m_Info.redirectStdErr)
        {
            if (pipe(m_StdErrPipe) < 0)
                cx_throw(ProcessException, "Failed to create stderr pipe: {}", std::strerror(errno));
        }
        if (m_Info.redirectStdIn)
        {
            if (pipe(m_StdInPipe) < 0)
                cx_throw(ProcessException, "Failed to create stdin pipe: {}", std::strerror(errno));
        }

        m_PID = fork();
        if (m_PID == -1)
            cx_throw(ProcessException, "POSIX: Failed to fork process: {}", std::strerror(errno));

        if (m_PID == 0)
        {
            // ---- Child process ----

            // Redirect stdout: child writes to the write end of the stdout pipe.
            if (m_Info.redirectStdOut)
            {
                close(m_StdOutPipe[0]); // Child doesn't read from stdout pipe.
                dup2(m_StdOutPipe[1], STDOUT_FILENO);
                close(m_StdOutPipe[1]);
            }

            // Redirect stderr: child writes to the write end of the stderr pipe.
            if (m_Info.redirectStdErr)
            {
                close(m_StdErrPipe[0]); // Child doesn't read from stderr pipe.
                dup2(m_StdErrPipe[1], STDERR_FILENO);
                close(m_StdErrPipe[1]);
            }

            // Redirect stdin: child reads from the read end of the stdin pipe.
            if (m_Info.redirectStdIn)
            {
                close(m_StdInPipe[1]); // Child doesn't write to stdin pipe.
                dup2(m_StdInPipe[0], STDIN_FILENO);
                close(m_StdInPipe[0]);
            }

            // Change working directory if requested.
            if (m_Info.cwd)
            {
                if (chdir(m_Info.cwd->c_str()) != 0)
                {
                    perror("chdir");
                    std::exit(EXIT_FAILURE);
                }
            }

            // Tokenize command into argv.
            // Use a mutable copy since strtok modifies the string.
            std::string cmd_copy = m_Info.command;
            char*       args[Process::MAX_ARG_COUNT];
            char*       token = std::strtok(cmd_copy.data(), " ");
            u32         i     = 0;
            while (token && i < Process::MAX_ARG_COUNT - 1)
            {
                args[i++] = token;
                token     = std::strtok(nullptr, " ");
            }
            args[i] = nullptr;

            execvp(args[0], args);

            // If we reach here, execvp failed.
            perror("execvp");
            std::exit(EXIT_FAILURE);
        }

        // ---- Parent process ----
        m_Running = true;

        // Close the pipe ends the parent doesn't use.
        if (m_Info.redirectStdOut)
            close(m_StdOutPipe[1]);
        if (m_Info.redirectStdErr)
            close(m_StdErrPipe[1]);
        if (m_Info.redirectStdIn)
            close(m_StdInPipe[0]);

        // Mark the write-end / read-end we closed as invalid so the destructor
        // doesn't double-close them.
        m_StdOutPipe[1] = -1;
        m_StdErrPipe[1] = -1;
        m_StdInPipe[0]  = -1;

        // A non-blocking thread that will wait for the process to finish,
        // get the exit code and indicate that the process has stopped.
        // Since the thread is detached, there's a chance that the owning ProcessHandle (which is just a
        // mem::Shared<Process>) might get out of scope before our thread finishes while this thread and our process are
        // still active. To fix this we can make the thread hold a strong reference to the process handle, we do this by
        // passing a dummy variable that creates an instance of our shared pointer using NewSharedFromThis() (because
        // Process inherits from mem::SharedManagable).
        std::thread(
            [self = NewSharedFromThis()]() mutable
            {
                auto* p = static_cast<POSIXProcess*>(self.Get());

                // Spawn reader threads so stdout and stderr are drained concurrently.
                // This prevents deadlocks when the child writes to both streams.
                if (p->m_Info.redirectStdOut)
                {
                    p->m_StdOutThread = std::thread(
                        [p]()
                        {
                            char    buffer[Process::READ_BUFFER_SIZE];
                            ssize_t bytes_read = 0;
                            while ((bytes_read = read(p->m_StdOutPipe[0], buffer, sizeof(buffer) - 1)) > 0)
                            {
                                buffer[bytes_read] = '\0';
                                if (p->Event_OnOutDataReceived)
                                    p->Event_OnOutDataReceived(buffer, static_cast<usize>(bytes_read));
                            }
                            close(p->m_StdOutPipe[0]);
                            p->m_StdOutPipe[0] = -1;
                        });
                }

                if (p->m_Info.redirectStdErr)
                {
                    p->m_StdErrThread = std::thread(
                        [p]()
                        {
                            char    buffer[Process::READ_BUFFER_SIZE];
                            ssize_t bytes_read = 0;
                            while ((bytes_read = read(p->m_StdErrPipe[0], buffer, sizeof(buffer) - 1)) > 0)
                            {
                                buffer[bytes_read] = '\0';
                                if (p->Event_OnErrDataReceived)
                                    p->Event_OnErrDataReceived(buffer, static_cast<usize>(bytes_read));
                            }
                            close(p->m_StdErrPipe[0]);
                            p->m_StdErrPipe[0] = -1;
                        });
                }

                // Wait for the child to exit.
                i32 status = 0;
                waitpid(p->m_PID, &status, 0);
                if (WIFEXITED(status))
                {
                    p->m_ExitCode = WEXITSTATUS(status);
                    p->m_Running  = false;

                    if (p->m_Info.onExit)
                        p->m_Info.onExit(p->m_ExitCode);
                }
                else
                {
                    p->m_ExitCode = -1;
                    p->m_Running  = false;

                    if (p->m_Info.onExit)
                        p->m_Info.onExit(p->m_ExitCode);
                }
            })
            .detach();
    }

    i32 POSIXProcess::WaitForExit()
    {
        if (m_PID != -1)
        {
            while (m_Running)
                std::this_thread::yield();
            return m_ExitCode;
        }
        cx_throw(InvalidOperationException, "Process hasn't been launched.");
    }

    void POSIXProcess::WriteLine(const std::string_view msg)
    {
        if (m_StdInPipe[1] != -1)
        {
            write(m_StdInPipe[1], msg.data(), msg.size());
            write(m_StdInPipe[1], "\n", 1);
        }
    }
} // namespace codex::sys
