#ifndef CODEX_CORE_EXCEPTION_H
#define CODEX_CORE_EXCEPTION_H

#include <sdafx.h>

#include <fmt/core.h>

#ifndef CX_CONFIG_DEBUG
template <typename T, typename... TArgs>
inline void cx_throw_trap(TArgs&&... args)
{
    CX_DEBUG_TRAP();
    throw T(std::forward<TArgs>(args)...);
}
#define cx_throw(ex_type, ...) cx_throw_trap<ex_type>(fmt::format(__VA_ARGS__), __FILE__, CX_PRETTY_FUNCTION, __LINE__)
#define cx_throwd(ex_type)     cx_throw_trap<ex_type>("", __FILE__, CX_PRETTY_FUNCTION, __LINE__)
#else
#define cx_throw(ex_type, ...) throw ex_type(fmt::format(__VA_ARGS__), __FILE__, CX_PRETTY_FUNCTION, __LINE__)
#define cx_throwd(ex_type)     throw ex_type("", __FILE__, CX_PRETTY_FUNCTION, __LINE__)
#endif
#define cx_except(ex_type, ...) ex_type(fmt::format(__VA_ARGS__), __FILE__, CX_PRETTY_FUNCTION, __LINE__)
#define cx_exceptd(ex_type)     ex_type("", __FILE__, CX_PRETTY_FUNCTION, __LINE__)
#define CX_CUSTOM_EXCEPTION(name, default_msg)                                                                         \
    class name : public CodexException                                                                                 \
    {                                                                                                                  \
        using CodexException::CodexException;                                                                          \
                                                                                                                       \
    public:                                                                                                            \
        const char* default_message() const noexcept override                                                          \
        {                                                                                                              \
            return default_msg;                                                                                        \
        }                                                                                                              \
    };
#define CX_CONSTRUCTOR(default_msg)                                                                                    \
    using CodexException::CodexException;                                                                              \
                                                                                                                       \
public:                                                                                                                \
    const char* default_message() const noexcept override \                                                            \
    {                                                                                                                  \
        \ return default_msg;                                                                                          \
        \                                                                                                              \
    }

namespace codex {
    class CODEX_API CodexException : public std::exception
    {
    protected:
        std::string m_Message;
        const char* m_File      = nullptr;
        const char* m_Function  = nullptr;
        const char* m_ClassName = nullptr;
        u32         m_Line      = 0;

    public:
        CodexException() noexcept = default;
        CodexException(const std::string_view message) noexcept;
        CodexException(const std::string_view message, const char* file, const char* function, const u32 line) noexcept;

    public:
        virtual const char* default_message() const noexcept { return "Unknown engine message."; }

    public:
        static inline std::string TypeNameDemangle(const char* name)
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
                               codex::CodexException::TypeNameDemangle(typeid(*this).name()), what(), backtrace());
        }

    public:
        friend auto operator<<(std::ostream& stream, const codex::CodexException& ex) noexcept -> std::ostream&
        {
            stream << fmt::format("An exception was caught: {}: {}\n\t{}",
                                  codex::CodexException::TypeNameDemangle(typeid(ex).name()), ex.what(),
                                  ex.backtrace());
            return stream;
        }
    };

    // Generic Exceptions
    CX_CUSTOM_EXCEPTION(IOException, "I/O operation did not succeed.")
    CX_CUSTOM_EXCEPTION(FileNotFoundException, "File was not found.")
    CX_CUSTOM_EXCEPTION(DirectoryNotFoundException, "Directory was not found.")
    CX_CUSTOM_EXCEPTION(NullReferenceException, "Object reference was not instantiated.")
    CX_CUSTOM_EXCEPTION(IndexOutOfBoundsException, "Index was out of bounds.")
    CX_CUSTOM_EXCEPTION(NotFoundException, "Item was not found.")
    CX_CUSTOM_EXCEPTION(InvalidArgumentException, "Provided argument was invalid.")
    CX_CUSTOM_EXCEPTION(InvalidOperationException, "Something somewhere went wrong, we're not sure why.")

    class CODEX_API NativeBehaviourException : public CodexException
    {
        // friend class NativeBehaviour; // TODO: Might be redundant.
        friend class NativeBehaviourComponent;

    private:
        CodexException m_InnerException;

    public:
        using CodexException::CodexException;

    public:
        const CodexException& InnerException() const noexcept { return m_InnerException; }
    };
} // namespace codex

#endif // CODEX_CORE_EXCEPTION_H
