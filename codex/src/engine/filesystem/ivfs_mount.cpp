#include "ivfs_mount.h"

#include <engine/core/engine.h>

namespace codex::fs {
    cc::Task<Shared<FileHandle>> IVFSMount::open_async(std::string path, FileProperties props) noexcept
    {
        co_await Engine::get_worker_pool();
        co_return open(path, props);
    }

    cc::Task<bool> IVFSMount::exists_async(std::string path) const noexcept
    {
        co_await Engine::get_worker_pool();
        co_return exists(path);
    }

    cc::Task<bool> IVFSMount::mkdir_async(std::string path) noexcept
    {
        co_await Engine::get_worker_pool();
        co_return mkdir(path);
    }

    cc::Task<std::vector<std::string>> IVFSMount::list_async(std::string path) const noexcept
    {
        co_await Engine::get_worker_pool();
        co_return list(path);
    }

    cc::Task<bool> IVFSMount::is_directory_async(std::string path) const noexcept
    {
        co_await Engine::get_worker_pool();
        co_return is_directory(path);
    }
} // namespace codex::fs
