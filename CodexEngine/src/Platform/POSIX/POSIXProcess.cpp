#include "POSIXProcess.h"

namespace codex::sys {
    POSIXProcess::POSIXProcess(ProcessInfo info) noexcept
        : Process(std::move(info))
    {
    }

    POSIXProcess::~POSIXProcess() noexcept
    {
        lgx::Get("engine").Log(lgx::Info, "~POSIX()");

        if (m_Info.redirectStdOut)
            close(m_StdOutPipe[0]);

        // Wait for our threads to finish.
        if (m_StdOutThread.joinable())
            m_StdOutThread.join();
        if (m_StdErrThread.joinable())
            m_StdErrThread.join();
    }

    void POSIXProcess::Launch()
    {
        // Create the process first.
        m_PID = fork();

        // Indicate that we've started.
        m_Running = true;

        // Create ze pipes.
        if (m_Info.redirectStdOut)
        {
            // Create the redirection pipe for stdout.
            if (pipe(m_StdOutPipe) == -1)
                perror("pipe(): ");

            // Create the redirection pipe for stderr.
            if (pipe(m_StdErrPipe) == -1)
                perror("pipe(): ");

            // Create the redirection pipe for stdin.
            if (pipe(m_StdInPipe) == -1)
                perror("pipe(): ");
        }

        // A non-blocking thread that will start the process, wait for the process to finish,
        // get the exit code and indicate that the process has stopped.
        // Since the thread is detached, there's a chance that the owning ProcessHandle (which is just a
        // mem::Shared<Process>) might get out of scope before our thread finishes while this thread and our process are
        // still active. To fix this we can make the thread hold a strong reference to the process handle, we do this by
        // passing a dummy variable that creates an instace of our shared pointer using NewSharedFromThis() (because
        // Process inherits from mem::SharedManagable).
        // Create the process.
        if (m_PID == -1)
            cx_throw(ProcessException, "POSIX: Failed to create process.");
        else if (m_PID == 0) // Child process.
        {
            if (m_Info.redirectStdOut)
            {
                // Close the read end of the pipe because the child does not read.
                close(m_StdOutPipe[0]);
                if (dup2(m_StdOutPipe[1], STDOUT_FILENO) < 0)
                    perror("dup2: "); // TODO: Throw an exception.

                // Close the write end?
                close(m_StdOutPipe[1]);
            }
            if (m_Info.redirectStdErr)
            {
                // Close the read end of the pipe because the child does not read.
                close(m_StdOutPipe[0]);
                if (dup2(m_StdOutPipe[1], STDERR_FILENO) < 0)
                    perror("dup2: "); // TODO: Throw an exception.

                // Close the write end?
                close(m_StdOutPipe[1]);
            }
            if (m_Info.redirectStdIn)
            {
                // Close the read end of the pipe because the child does not read.
                close(m_StdInPipe[1]);
                if (dup2(m_StdOutPipe[0], STDIN_FILENO) < 0)
                    perror("dup2: "); // TODO: Throw an exception.

                // Close the write end?
                close(m_StdOutPipe[0]);
            }

            // Execute.
            // Use the exec() syscall
            char* cmd = (char*)m_Info.command.c_str();
            char* args[Process::MAX_ARG_COUNT];
            char* token = std::strtok(cmd, " ");
            u32   i     = 0;
            while (token)
            {
                args[i++] = token;
                token     = std::strtok(nullptr, " ");
            }

            // Null terminate the stringn argument array.
            args[i] = nullptr;

            // Execute our command.
            if (execvp(args[0], args) < 0)
                cx_throw(ProcessException, "Native Error: {}", std::strerror(errno));

            // If we reach here then execvp() failed.
            std::exit(EXIT_FAILURE);
        }
        else // Parent process.
        {
            if (m_Info.redirectStdOut)
            {
                // Close the write end of the pipe because the parent does not write.
                close(m_StdOutPipe[1]);

                char    buffer[Process::READ_BUFFER_SIZE];
                ssize_t bytes_read = 0;
                while ((bytes_read = read(m_StdOutPipe[0], buffer, sizeof(buffer))) > 0)
                {
                    std::printf("out: %s\n", buffer);
                    this->Event_OnOutDataReceived(buffer, bytes_read);
                }
            }
            if (m_Info.redirectStdErr)
            {
                // Close the write end of the pipe because the parent does not write.
                close(m_StdErrPipe[1]);

                char    buffer[Process::READ_BUFFER_SIZE];
                ssize_t bytes_read = 0;
                while ((bytes_read = read(m_StdErrPipe[0], buffer, sizeof(buffer))) > 0)
                {
                    std::printf("out: %s\n", buffer);
                    this->Event_OnErrDataReceived(buffer, bytes_read);
                }
            }

            // Wait for the process to exit and store the exit code.
            waitpid(m_PID, &m_ExitCode, 0);
            if (WIFEXITED(m_ExitCode))
            {
                // Indicate that we're not running anymore.
                m_Running  = false;
                m_ExitCode = WEXITSTATUS(m_ExitCode);

                // If the user has subscribed to the exit event.
                if (m_Info.onExit)
                    m_Info.onExit(m_ExitCode);
            }
            else
                cx_throw(ProcessException, "POSIX: Process exited abnormally. Native Error: {}", std::strerror(errno));
        }
    }

    i32 POSIXProcess::WaitForExit()
    {
        if (m_PID != -1)
        {
            while (m_Running)
                ;
            return m_ExitCode;
        }
        cx_throw(InvalidOperationException, "Process hasn't been launched.");
    }

    void POSIXProcess::WriteLine(const std::string_view msg)
    {
        // Write to stdin?
    }
} // namespace codex::sys
