#pragma once

#include <engine/concurrency/public/task.h>
#include <engine/core/public/uuid.h>

namespace codex::fs {
    enum class FileMode : u8
    {
        Read   = bit(0), // open for reading
        Write  = bit(1), // open for writing (overwrite at cursor)
        Append = bit(2), // writes always go to end
        Create = bit(3), // create file if it doesn't exist
        Trunc  = bit(4), // clear contents on open
    };
    CX_ENABLE_BITWISE_ENUM(FileMode);

    struct FileProperties
    {
        FileMode mode = FileMode::Read;
    };

    class FileHandle
    {
    public:
        virtual ~FileHandle() noexcept { flush(); }

    public:
        virtual usize                     read(void* dest, const usize len) noexcept       = 0;
        virtual usize                     write(const void* src, const usize len) noexcept = 0;
        virtual void                      seek(const usize offset) noexcept                = 0;
        [[nodiscard]] virtual usize       tell() const noexcept                            = 0;
        [[nodiscard]] virtual usize       size() const noexcept                            = 0;
        [[nodiscard]] virtual std::string path() const noexcept                            = 0;
        [[nodiscard]] virtual u64         last_modified() const noexcept                   = 0;
        virtual void                      flush() {}

        virtual usize read_at(void* dest, usize len, usize offset) noexcept;
        virtual usize write_at(const void* src, usize len, usize offset) noexcept;

        cc::task<usize> read_async(void* dest, usize len) noexcept;
        cc::task<usize> write_async(const void* src, usize len) noexcept;
        cc::task<usize> read_at_async(void* dest, usize len, usize offset) noexcept;
        cc::task<usize> write_at_async(const void* src, usize len, usize offset) noexcept;
    };
} // namespace codex::fs
