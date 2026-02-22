#pragma once

namespace codex::fs {
    enum class FileMode
    {
        Read      = 1,
        Write     = 1 << 1,
        Append    = 1 << 2,
        ReadWrite = Write | Read,
    };

    enum class FilePermissions
    {
        Read    = 1,
        Write   = 1 << 1,
        Execute = 1 << 2,
        RWX     = Read | Write | Execute,
    };

    struct FileProperties
    {
        FileMode        mode  = FileMode::ReadWrite;
        FilePermissions perms = FilePermissions::RWX;
    };

    class FileHandle
    {
    public:
        ~FileHandle() noexcept = default;

    public:
        virtual usize Read(void* buffer, const usize len)  = 0;
        virtual usize Write(void* buffer, const usize len) = 0;
        virtual void  Seek(const usize offset)             = 0;
        virtual usize Tell()                               = 0;
        virtual usize Size()                               = 0;
        virtual void  Flush() {}

    public:
        virtual FileHandle& operator<<(const std::vector<u8>& src) = 0;
        virtual FileHandle& operator>>(std::vector<u8>& dest)      = 0;
    };
} // namespace codex::fs
