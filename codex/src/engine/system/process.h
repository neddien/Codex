#ifndef CODEX_SYSTEM_PROCESS_H
#define CODEX_SYSTEM_PROCESS_H

#include <engine/core/public/exception.h>
#include <engine/memory/public/memory.h>

namespace codex::sys {
    CX_CUSTOM_EXCEPTION(ProcessException, "Could not create process.")

    // An enum for the state of the process window.
    enum class WindowState
    {
        Normal,
        Maximized,
        Minimized,
        Hidden
    };

    struct ProcessInfo
    {
        std::string                command;
        opt<std::string> cwd              = std::nullopt;
        bool                       redirect_stdin   = false;
        bool                       redirect_stdout  = false;
        bool                       redirect_stderr  = false;
        bool                       system_shell     = true;
        bool                       create_window    = true;
        bool                       separate_console = false;
        bool                       shell_open       = false;
        bool                       detached         = false;
        WindowState                window_state     = WindowState::Normal;
        std::function<void(i32)>   on_exit          = nullptr;
    };

    class CODEX_API Process : public std::enable_shared_from_this<Process>
    {
    public:
        using ProcessHandle = Shared<Process>;

    protected:
        ProcessInfo info_;

    public:
        std::function<void(const char*, usize)> on_out_data_received;
        std::function<void(const char*, usize)> on_err_data_received;

    public:
        constexpr static auto READ_BUFFER_SIZE = 4096;
        constexpr static auto MAX_ARG_COUNT    = 128;

    protected:
        Process() = default;

    protected:
        explicit Process(ProcessInfo info) noexcept;
        virtual ~Process() = default;

    public:
        static ProcessHandle create(ProcessInfo info) noexcept;

    public:
        virtual void launch()                               = 0;
        virtual i32  wait_for_exit()                        = 0;
        virtual void write_line(const std::string_view msg) = 0;
    };
} // namespace codex::sys

#endif // CODEX_SYSTEM_PROCESS_H
