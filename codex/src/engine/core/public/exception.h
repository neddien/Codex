#pragma once

#include <engine/core/public/common_third_party_libs.h>

#define CX_CUSTOM_EXCEPTION(name, default_msg)                                                                         \
    class name : public CodexException                                                                                 \
    {                                                                                                                  \
        using CodexException::CodexException;                                                                          \
                                                                                                                       \
    public:                                                                                                            \
        const char* default_message() const noexcept override { return default_msg; }                                  \
    };
#define CX_CONSTRUCTOR(default_msg)                                                                                    \
    using CodexException::CodexException;                                                                              \
                                                                                                                       \
public:                                                                                                                \
    const char* default_message() const noexcept override \                                                              \
    {                                                                                                                  \
        \ return default_msg;                                                                                          \
        \                                                                                                              \
    }

namespace codex {
    // This wraps argument pack for fmt::format and also implicitly constructs a
    // std::source_location at ctor call site without using any macros which is pretty cool.
    struct FmtStringWithLoc
    {
        fmt::string_view     fmt;
        std::source_location loc;

        template <typename S>
        FmtStringWithLoc(const S& s, std::source_location location = std::source_location::current()) noexcept
            : fmt{ s }
            , loc{ location }
        {
        }
    };

    class CODEX_API CodexException : public std::exception
    {
    public:
        CodexException() noexcept = default;
        CodexException(const std::string_view message,
                       std::source_location   loc = std::source_location::current()) noexcept;

        template <typename... TArgs>
        CodexException(FmtStringWithLoc fmt_loc, TArgs&&... args) noexcept
            : message_{ fmt::vformat(fmt_loc.fmt, fmt::make_format_args(args...)) }
            , location_{ fmt_loc.loc }
        {
        }

    public:
        virtual const char* default_message() const noexcept { return "Unknown engine exception"; }

    public:
        static inline std::string type_name_demangle(const char* name)
        {
#if CX_PLATFORM_UNIX
            int                                    status = 0;
            std::unique_ptr<char, void (*)(void*)> res{ abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
            return (status == 0) ? res.get() : name;
#else
            return name;
#endif
        }

    public:
        [[nodiscard]] auto what() const noexcept -> const char* override;
        [[nodiscard]] auto backtrace() const noexcept -> std::string;
        [[nodiscard]] auto to_string() const noexcept -> std::string
        {
            return fmt::format("An exception was caught: {}: {}\n\t{}",
                               codex::CodexException::type_name_demangle(typeid(*this).name()), what(), backtrace());
        }

    public:
        friend auto operator<<(std::ostream& stream, const codex::CodexException& ex) noexcept -> std::ostream&
        {
            stream << fmt::format("An exception was caught: {}: {}\n\t{}",
                                  codex::CodexException::type_name_demangle(typeid(ex).name()), ex.what(),
                                  ex.backtrace());
            return stream;
        }

    protected:
        std::string          message_;
        std::source_location location_;
    };

    // Generic Exceptions
    CX_CUSTOM_EXCEPTION(IOException, "I/O operation did not succeed")
    CX_CUSTOM_EXCEPTION(FileNotFoundException, "No such file or directory")
    CX_CUSTOM_EXCEPTION(NullReferenceException, "Object reference was not instantiated")
    CX_CUSTOM_EXCEPTION(IndexOutOfBoundsException, "Index was out of bounds")
    CX_CUSTOM_EXCEPTION(NotFoundException, "Item was not found")
    CX_CUSTOM_EXCEPTION(InvalidArgumentException, "Provided argument was invalid")
    CX_CUSTOM_EXCEPTION(InvalidOperationException, "Something somewhere went wrong, we're not sure why")

    class CODEX_API NativeBehaviourException : public CodexException
    {
        // friend class NativeBehaviour; // TODO: Might be redundant.
        friend struct NativeBehaviourComponent;

    private:
        CodexException inner_exception_;

    public:
        using CodexException::CodexException;

    public:
        const CodexException& inner_exception() const noexcept { return inner_exception_; }
    };
} // namespace codex
