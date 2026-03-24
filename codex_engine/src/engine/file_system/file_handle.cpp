#include "file_handle.h"

#include <engine/core/engine.h>

namespace codex::fs {
    // Fallback: not thread-safe across concurrent read_at/write_at calls.
    // Platform subclasses override this with pread/pwrite or OVERLAPPED.
    usize FileHandle::read_at(void* dest, const usize len, const usize offset) noexcept
    {
        const usize saved = tell();
        seek(offset);
        const usize result = read(dest, len);
        seek(saved);
        return result;
    }

    usize FileHandle::write_at(const void* src, const usize len, const usize offset) noexcept
    {
        const usize saved = tell();
        seek(offset);
        const usize result = write(src, len);
        seek(saved);
        return result;
    }

    cc::Task<usize> FileHandle::read_async(void* dest, usize len) noexcept
    {
        co_await Engine::get_worker_pool();
        co_return read(dest, len);
    }

    cc::Task<usize> FileHandle::write_async(const void* src, usize len) noexcept
    {
        co_await Engine::get_worker_pool();
        co_return write(src, len);
    }

    cc::Task<usize> FileHandle::read_at_async(void* dest, usize len, usize offset) noexcept
    {
        co_await Engine::get_worker_pool();
        co_return read_at(dest, len, offset);
    }

    cc::Task<usize> FileHandle::write_at_async(const void* src, usize len, usize offset) noexcept
    {
        co_await Engine::get_worker_pool();
        co_return write_at(src, len, offset);
    }
} // namespace codex::fs
