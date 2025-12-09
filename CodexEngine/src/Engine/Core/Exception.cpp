#include "./Public/Exception.h"

namespace codex {
    CodexException::CodexException(const std::string_view message) noexcept
        : m_Message(message)
    {
    }

    CodexException::CodexException(const std::string_view message, const char* file, const char* function,
                                   const u32 line) noexcept
        : m_Message(message)
        , m_File(file)
        , m_Function(function)
        , m_Line(line)
    {
    }

    const char* CodexException::what() const noexcept
    {
        return (!m_Message.empty()) ? m_Message.c_str() : default_message();
    }

    std::string CodexException::backtrace() const noexcept
    {
        return fmt::format("at {} in {} line: {}", m_Function, m_File, m_Line);
    }
} // namespace codex
