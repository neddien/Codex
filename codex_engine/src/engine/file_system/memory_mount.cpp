#include "memory_mount.h"

#include "file_handle.h"
#include "public/file_system.h"

namespace codex::fs {
    class MemoryFileHandle : public FileHandle
    {
    public:
        MemoryFileHandle(const std::string_view path, std::vector<u8>& data, std::shared_mutex& data_mutex,
                         mem::Shared<MemoryMount> owner, const FileProperties props = FileProperties{})
            : path_{ normalize(std::string{ path }) }
            , props_{ props }
            , data_{ data }
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
                const usize avail    = data_.size() - cursor_;
                const usize read_len = std::min(avail, len);

                if (read_len) {
                    // TODO: Calculate read_len based on end addr returned by memcpy instead of assuming that you wrote
                    // read_len bytes.
                    [[maybe_unused]] void* end_buf = std::memcpy(dest, data_.data() + cursor_, len);
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
                    cursor_ = data_.size();

                const usize total_len = cursor_ + len;
                if (data_.size() < total_len)
                    data_.resize(total_len);

                std::memcpy(data_.data() + cursor_, src, len);
                cursor_ += len;

                return len;
            }

            return 0;
        }
        usize read_at(void* dest, const usize len, const usize offset) noexcept override
        {
            std::shared_lock data_lock{ data_mutex_ };

            if (!(props_.mode & FileMode::Read))
                return 0;

            const usize avail    = data_.size() > offset ? data_.size() - offset : 0;
            const usize read_len = std::min(avail, len);
            if (read_len)
                std::memcpy(dest, data_.data() + offset, read_len);
            return read_len;
        }

        usize write_at(const void* src, const usize len, const usize offset) noexcept override
        {
            std::scoped_lock data_lock{ data_mutex_ };

            if (!(props_.mode & (FileMode::Write | FileMode::Append)))
                return 0;

            const usize end = offset + len;
            if (data_.size() < end)
                data_.resize(end);
            std::memcpy(data_.data() + offset, src, len);
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
            return data_.size();
        }
        std::string path() const noexcept override
        {
            std::shared_lock handle_lock{ mutex_ };
            return path_;
        }

    private:
        std::vector<u8>&          data_;
        std::shared_mutex&        data_mutex_;
        mutable std::shared_mutex mutex_;
        std::string               path_;
        FileProperties            props_;
        usize                     cursor_;
        mem::Shared<MemoryMount>  owner_;
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

    mem::Shared<FileHandle> MemoryMount::open(const std::string& path, const FileProperties props) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        const auto npath   = normalize(path);
        auto       file_it = files_.find(npath);

        if (file_it == files_.end()) {
            if (!(props.mode & FileMode::Create))
                return nullptr;

            auto [it, _] = files_.try_emplace(npath);
            return mem::Shared<MemoryFileHandle>::make(npath, it->second.buffer, it->second.mutex,
                                                       new_shared_from_this(), props);
        }

        FileEntry& entry = file_it->second;

        if ((props.mode & (FileMode::Write | FileMode::Append)) && entry.read_only)
            return nullptr;

        if (props.mode & FileMode::Trunc)
            entry.buffer.clear();

        auto mfh = mem::Shared<MemoryFileHandle>::make(npath, entry.buffer, entry.mutex, new_shared_from_this(), props);
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
        std::string current;
        for (const auto& c : util::str::split(npath, '/')) {
            if (!current.empty())
                current += '/';
            current += c;
            dirs_.insert(current);
        }

        return true;
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
