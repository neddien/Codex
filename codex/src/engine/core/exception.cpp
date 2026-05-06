#include "public/exception.h"

namespace codex {
    CodexException::CodexException(const std::string_view message, std::source_location loc) noexcept
        : message_{ message }
        , location_{ loc }
    {
    }

    const char* CodexException::what() const noexcept
    {
        return (!message_.empty()) ? message_.c_str() : default_message();
    }

    std::string CodexException::backtrace() const noexcept
    {
        return fmt::format("at {} in {} line: {}", location_.function_name(), location_.file_name(), location_.line());
    }
} // namespace codex
