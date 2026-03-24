#include "posix_process.h"

namespace codex::sys {
    POSIXProcess::POSIXProcess(ProcessInfo info) noexcept
        : Process(std::move(info))
    {
    }

    POSIXProcess::~POSIXProcess() noexcept
    {
        // Close any pipe fds the parent still holds open.
        if (stdout_pipe_[0] != -1)
            close(stdout_pipe_[0]);
        if (stderr_pipe_[0] != -1)
            close(stderr_pipe_[0]);
        if (stdin_pipe_[1] != -1)
            close(stdin_pipe_[1]);

        // Wait for reader threads to finish.
        if (stdout_thread_.joinable())
            stdout_thread_.join();
        if (stderr_thread_.joinable())
            stderr_thread_.join();
    }

    void POSIXProcess::launch()
    {
        // Create pipes BEFORE fork so both parent and child inherit the fds.
        if (info_.redirect_stdout)
        {
            if (pipe(stdout_pipe_) < 0)
                throw ProcessException("Failed to create stdout pipe: {}", std::strerror(errno));
        }
        if (info_.redirect_stderr)
        {
            if (pipe(stderr_pipe_) < 0)
                throw ProcessException("Failed to create stderr pipe: {}", std::strerror(errno));
        }
        if (info_.redirect_stdin)
        {
            if (pipe(stdin_pipe_) < 0)
                throw ProcessException("Failed to create stdin pipe: {}", std::strerror(errno));
        }

        pid_ = fork();
        if (pid_ == -1)
            throw ProcessException("POSIX: Failed to fork process: {}", std::strerror(errno));

        if (pid_ == 0)
        {
            // ---- Child process ----

            // Redirect stdout: child writes to the write end of the stdout pipe.
            if (info_.redirect_stdout)
            {
                close(stdout_pipe_[0]); // Child doesn't read from stdout pipe.
                dup2(stdout_pipe_[1], STDOUT_FILENO);
                close(stdout_pipe_[1]);
            }

            // Redirect stderr: child writes to the write end of the stderr pipe.
            if (info_.redirect_stderr)
            {
                close(stderr_pipe_[0]); // Child doesn't read from stderr pipe.
                dup2(stderr_pipe_[1], STDERR_FILENO);
                close(stderr_pipe_[1]);
            }

            // Redirect stdin: child reads from the read end of the stdin pipe.
            if (info_.redirect_stdin)
            {
                close(stdin_pipe_[1]); // Child doesn't write to stdin pipe.
                dup2(stdin_pipe_[0], STDIN_FILENO);
                close(stdin_pipe_[0]);
            }

            // Change working directory if requested.
            if (info_.cwd)
            {
                if (chdir(info_.cwd->c_str()) != 0)
                {
                    perror("chdir");
                    std::exit(EXIT_FAILURE);
                }
            }

            // Tokenize command into argv.
            // Use a mutable copy since strtok modifies the string.
            std::string cmd_copy = info_.command;
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
        running_ = true;

        // Close the pipe ends the parent doesn't use.
        if (info_.redirect_stdout)
            close(stdout_pipe_[1]);
        if (info_.redirect_stderr)
            close(stderr_pipe_[1]);
        if (info_.redirect_stdin)
            close(stdin_pipe_[0]);

        // Mark the write-end / read-end we closed as invalid so the destructor
        // doesn't double-close them.
        stdout_pipe_[1] = -1;
        stderr_pipe_[1] = -1;
        stdin_pipe_[0]  = -1;

        // A non-blocking thread that will wait for the process to finish,
        // get the exit code and indicate that the process has stopped.
        // Since the thread is detached, there's a chance that the owning ProcessHandle (which is just a
        // mem::Shared<Process>) might get out of scope before our thread finishes while this thread and our process are
        // still active. To fix this we can make the thread hold a strong reference to the process handle, we do this by
        // passing a dummy variable that creates an instance of our shared pointer using NewSharedFromThis() (because
        // Process inherits from mem::SharedManagable).
        std::thread(
            [self = new_shared_from_this()]() mutable
            {
                auto* p = static_cast<POSIXProcess*>(self.get());

                // Spawn reader threads so stdout and stderr are drained concurrently.
                // This prevents deadlocks when the child writes to both streams.
                if (p->info_.redirect_stdout)
                {
                    p->stdout_thread_ = std::thread(
                        [p]()
                        {
                            char    buffer[Process::READ_BUFFER_SIZE];
                            ssize_t bytes_read = 0;
                            while ((bytes_read = read(p->stdout_pipe_[0], buffer, sizeof(buffer) - 1)) > 0)
                            {
                                buffer[bytes_read] = '\0';
                                if (p->on_out_data_received)
                                    p->on_out_data_received(buffer, static_cast<usize>(bytes_read));
                            }
                            close(p->stdout_pipe_[0]);
                            p->stdout_pipe_[0] = -1;
                        });
                }

                if (p->info_.redirect_stderr)
                {
                    p->stderr_thread_ = std::thread(
                        [p]()
                        {
                            char    buffer[Process::READ_BUFFER_SIZE];
                            ssize_t bytes_read = 0;
                            while ((bytes_read = read(p->stderr_pipe_[0], buffer, sizeof(buffer) - 1)) > 0)
                            {
                                buffer[bytes_read] = '\0';
                                if (p->on_err_data_received)
                                    p->on_err_data_received(buffer, static_cast<usize>(bytes_read));
                            }
                            close(p->stderr_pipe_[0]);
                            p->stderr_pipe_[0] = -1;
                        });
                }

                // Wait for the child to exit.
                i32 status = 0;
                waitpid(p->pid_, &status, 0);
                if (WIFEXITED(status))
                {
                    p->exit_code_ = WEXITSTATUS(status);
                    p->running_   = false;

                    if (p->info_.on_exit)
                        p->info_.on_exit(p->exit_code_);
                } else {
                    p->exit_code_ = -1;
                    p->running_   = false;

                    if (p->info_.on_exit)
                        p->info_.on_exit(p->exit_code_);
                }
            })
            .detach();
    }

    i32 POSIXProcess::wait_for_exit()
    {
        if (pid_ != -1)
        {
            while (running_)
                std::this_thread::yield();
            return exit_code_;
        }
        throw InvalidOperationException("Process hasn't been launched.");
    }

    void POSIXProcess::write_line(const std::string_view msg)
    {
        if (stdin_pipe_[1] != -1)
        {
            write(stdin_pipe_[1], msg.data(), msg.size());
            write(stdin_pipe_[1], "\n", 1);
        }
    }
} // namespace codex::sys
