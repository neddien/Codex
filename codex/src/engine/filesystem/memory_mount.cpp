#include "memory_mount.h"

#include "public/file_handle.h"
#include "public/filesystem.h"

namespace codex::fs {
    class MemoryFileHandle : public FileHandle
    {
    public:
        MemoryFileHandle(const std::string_view path, MemoryMount::FileEntry& entry, std::shared_mutex& data_mutex,
                         Shared<MemoryMount> owner, const FileProperties props = FileProperties{})
            : path_{ normalize(std::string{ path }) }
            , props_{ props }
            , entry_{ entry }
            , cursor_{ 0 }
            , data_mutex_{ data_mutex }
            , mutex_{}
            , owner_{ std::move(owner) }
        {
        }

    public:
        usize read(void* dest, const usize len) noexcept override
        {
            std::scoped_lock handle_lock{ mutex_ };
            std::shared_lock data_lock{ data_mutex_ };

            if (props_.mode & FileMode::Read) {
                const usize avail    = entry_.buffer.size() - cursor_;
                const usize read_len = std::min(avail, len);

                if (read_len) {
                    // TODO: Calculate read_len based on end addr returned by memcpy instead of assuming that you wrote
                    // read_len bytes.
                    [[maybe_unused]] void* end_buf = std::memcpy(dest, entry_.buffer.data() + cursor_, len);
                    cursor_ += read_len;
                }

                return read_len;
            }

            return 0;
        }
        usize write(const void* src, const usize len) noexcept override
        {
            std::scoped_lock handle_lock{ mutex_ };
            std::scoped_lock data_lock{ data_mutex_ };

            if (props_.mode & (FileMode::Write | FileMode::Append)) {
                if (props_.mode & FileMode::Append)
                    cursor_ = entry_.buffer.size();

                const usize total_len = cursor_ + len;
                if (entry_.buffer.size() < total_len)
                    entry_.buffer.resize(total_len);

                std::memcpy(entry_.buffer.data() + cursor_, src, len);
                cursor_ += len;

                entry_.last_modif = util::chron::unix_epoch_now();

                return len;
            }

            return 0;
        }
        usize read_at(void* dest, const usize len, const usize offset) noexcept override
        {
            std::shared_lock data_lock{ data_mutex_ };

            if (!(props_.mode & FileMode::Read))
                return 0;

            const usize avail    = entry_.buffer.size() > offset ? entry_.buffer.size() - offset : 0;
            const usize read_len = std::min(avail, len);
            if (read_len)
                std::memcpy(dest, entry_.buffer.data() + offset, read_len);
            return read_len;
        }

        usize write_at(const void* src, const usize len, const usize offset) noexcept override
        {
            std::scoped_lock data_lock{ data_mutex_ };

            if (!(props_.mode & (FileMode::Write | FileMode::Append)))
                return 0;

            const usize end = offset + len;
            if (entry_.buffer.size() < end)
                entry_.buffer.resize(end);
            std::memcpy(entry_.buffer.data() + offset, src, len);

            entry_.last_modif = util::chron::unix_epoch_now();

            return len;
        }

        void seek(const usize offset) noexcept override
        {
            std::scoped_lock handle_lock{ mutex_ };
            cursor_ = offset;
        }
        usize tell() const noexcept override
        {
            std::shared_lock handle_lock{ mutex_ };
            return cursor_;
        }
        usize size() const noexcept override
        {
            std::shared_lock data_lock{ data_mutex_ };
            return entry_.buffer.size();
        }
        std::string path() const noexcept override
        {
            std::shared_lock handle_lock{ mutex_ };
            return path_;
        }
        u64 last_modified() const noexcept override
        {
            std::shared_lock data_lock{ data_mutex_ };
            return entry_.last_modif;
        }

    private:
        MemoryMount::FileEntry&   entry_;
        std::shared_mutex&        data_mutex_;
        mutable std::shared_mutex mutex_;
        std::string               path_;
        FileProperties            props_;
        usize                     cursor_;
        Shared<MemoryMount>       owner_;
    };

    MemoryMount::MemoryMount(const i32 priority)
        : priority_{ priority }
        , mutex_{}
    {
    }

    bool MemoryMount::exists(const std::string& path) const noexcept
    {
        std::shared_lock lock{ mutex_ };

        const auto npath = normalize(path);
        return files_.contains(npath) || dirs_.contains(npath);
    }

    Shared<FileHandle> MemoryMount::open(const std::string& path, const FileProperties props) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        const auto npath   = normalize(path);
        auto       file_it = files_.find(npath);

        if (file_it == files_.end()) {
            if (!(props.mode & FileMode::Create))
                return nullptr;

            auto [it, _] = files_.try_emplace(npath);
            return Shared<MemoryFileHandle>::make(npath, it->second, it->second.mutex, shared_from_this(), props);
        }

        FileEntry& entry = file_it->second;

        if ((props.mode & (FileMode::Write | FileMode::Append)) && entry.read_only)
            return nullptr;

        if (props.mode & FileMode::Trunc)
            entry.buffer.clear();

        auto mfh = Shared<MemoryFileHandle>::make(npath, entry, entry.mutex, shared_from_this(), props);
        if (props.mode & FileMode::Append)
            mfh->seek(mfh->size());

        return mfh;
    }

    i32 MemoryMount::priority() const noexcept
    {
        std::shared_lock lock{ mutex_ };
        return priority_;
    }

    bool MemoryMount::mkdir(const std::string& rel_path) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        const auto npath = normalize(rel_path);
        if (npath.empty())
            return true;

        // Insert all ancestors so that e.g. mkdir("a/b/c") implicitly creates "a" and "a/b".
        if (!files_.contains(rel_path)) {
            std::string current;
            for (const auto& c : util::str::split(npath, '/')) {
                if (!current.empty())
                    current += '/';
                current += c;
                dirs_.insert(current);
            }

            return true;
        }
        return false;
    }

    bool MemoryMount::rm(const std::string& rel_path) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        const auto npath = normalize(rel_path);
        if (npath.empty())
            return true;

        if (auto it = files_.find(npath); it != files_.end()) {
            files_.erase(it);
            return true;
        } else if (auto it = dirs_.find(npath); it != dirs_.end()) {
            dirs_.erase(it);
            return true;
        }

        return false;
    }

    std::vector<std::string> MemoryMount::list(const std::string& rel_path, const ListOptions opts) const noexcept
    {
        std::shared_lock lock{ mutex_ };

        const auto npath      = normalize(rel_path);
        const auto prefix     = npath.empty() ? std::string{} : npath + '/';
        const bool recursive  = !!(opts & ListOptions::Recursive);
        const bool files_only = !!(opts & ListOptions::FilesOnly);
        const bool dirs_only  = !!(opts & ListOptions::DirsOnly);

        std::unordered_set<std::string> result;

        if (!dirs_only) {
            for (const auto& [key, _] : files_) {
                if (!prefix.empty() && !key.starts_with(prefix))
                    continue;
                const auto rest = key.substr(prefix.size());
                if (recursive) {
                    result.insert(rest);
                } else {
                    const auto slash = rest.find('/');
                    if (slash == std::string::npos)
                        result.insert(rest);
                }
            }
        }

        if (!files_only) {
            for (const auto& dir : dirs_) {
                if (!prefix.empty() && !dir.starts_with(prefix))
                    continue;
                const auto rest = dir.substr(prefix.size());
                if (recursive) {
                    result.insert(rest);
                } else {
                    const auto slash = rest.find('/');
                    if (slash == std::string::npos)
                        result.insert(rest);
                }
            }
        }

        return { result.begin(), result.end() };
    }

    bool MemoryMount::is_directory(const std::string& rel_path) const noexcept
    {
        std::shared_lock lock{ mutex_ };

        const auto npath = normalize(rel_path);

        if (dirs_.contains(npath))
            return true;

        const auto prefix = npath + '/';
        for (const auto& [key, _] : files_) {
            if (key.starts_with(prefix))
                return true;
        }

        return false;
    }
} // namespace codex::fs
