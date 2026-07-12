#include "memory_mount.h"

#include "public/file_handle.h"
#include "public/filesystem.h"

namespace codex::fs {
    namespace stdfs = std::filesystem;

    namespace {
        [[nodiscard]] bool is_under(std::string_view path, std::string_view root) noexcept
        {
            if (path == root)
                return true;

            if (path.size() <= root.size())
                return false;

            return path.starts_with(root) && path[root.size()] == '/';
        }
    } // namespace

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
                const usize avail    = entry_.desc.buffer.size() - cursor_;
                const usize read_len = std::min(avail, len);

                if (read_len) {
                    // TODO: Calculate read_len based on end addr returned by memcpy instead of assuming that you wrote
                    // read_len bytes.
                    [[maybe_unused]] void* end_buf = std::memcpy(dest, entry_.desc.buffer.data() + cursor_, len);
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
                    cursor_ = entry_.desc.buffer.size();

                const usize total_len = cursor_ + len;
                if (entry_.desc.buffer.size() < total_len)
                    entry_.desc.buffer.resize(total_len);

                std::memcpy(entry_.desc.buffer.data() + cursor_, src, len);
                cursor_ += len;

                entry_.desc.last_modif = util::chron::unix_epoch_now();

                return len;
            }

            return 0;
        }
        usize read_at(void* dest, const usize len, const usize offset) noexcept override
        {
            std::shared_lock data_lock{ data_mutex_ };

            if (!(props_.mode & FileMode::Read))
                return 0;

            const usize avail    = entry_.desc.buffer.size() > offset ? entry_.desc.buffer.size() - offset : 0;
            const usize read_len = std::min(avail, len);
            if (read_len)
                std::memcpy(dest, entry_.desc.buffer.data() + offset, read_len);
            return read_len;
        }

        usize write_at(const void* src, const usize len, const usize offset) noexcept override
        {
            std::scoped_lock data_lock{ data_mutex_ };

            if (!(props_.mode & (FileMode::Write | FileMode::Append)))
                return 0;

            const usize end = offset + len;
            if (entry_.desc.buffer.size() < end)
                entry_.desc.buffer.resize(end);
            std::memcpy(entry_.desc.buffer.data() + offset, src, len);

            entry_.desc.last_modif = util::chron::unix_epoch_now();

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
            return entry_.desc.buffer.size();
        }
        std::string path() const noexcept override
        {
            std::shared_lock handle_lock{ mutex_ };
            return path_;
        }
        u64 last_modified() const noexcept override
        {
            std::shared_lock data_lock{ data_mutex_ };
            return entry_.desc.last_modif;
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

        const std::string npath = normalize(path);
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

            auto [it, did_insert] = files_.try_emplace(npath, FileEntry{});
            assert(did_insert);
            return Shared<MemoryFileHandle>::make(npath, it->second, it->second.mutex, shared_from_this(), props);
        }

        FileEntry& entry = file_it->second;

        if ((props.mode & (FileMode::Write | FileMode::Append)) && entry.desc.read_only)
            return nullptr;

        if (props.mode & FileMode::Trunc)
            entry.desc.buffer.clear();

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
            return false;

        if (auto it = files_.find(npath); it != files_.end()) {
            files_.erase(it);
            return true;
        } else if (auto it = dirs_.find(npath); it != dirs_.end()) {
            dirs_.erase(it);
            return true;
        }

        return false;
    }

    bool MemoryMount::cp(const std::string& src_rel_path, const std::string& dst_rel_path,
                         const bool recursive) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        const stdfs::path nspath     = normalize(src_rel_path);
        const std::string nspath_str = nspath.generic_string();
        const stdfs::path ndpath     = normalize(dst_rel_path);
        const std::string ndpath_str = ndpath.generic_string();

        if (nspath.empty() || ndpath.empty())
            return false;

        if (!exists(ndpath.parent_path().generic_string()) && !exists(nspath.generic_string()))
            return false;

        // /a/file.t /b/
        // /a/file.t /a/file2.t (also includes replace)
        // /a/       /b/
        // /a/       /b/c

        if (auto sit = files_.find(nspath_str); sit != files_.end()) {
            std::shared_lock guard{ sit->second.mutex };

            // f : f (replace)
            if (auto dit = files_.find(ndpath_str); dit != files_.end()) {
                std::scoped_lock guard{ dit->second.mutex };

                dit->second.desc = sit->second.desc;
                return true;
            }
            // f : d
            else if (auto dit = dirs_.find(ndpath_str); dit != dirs_.end()) {
                files_.emplace((ndpath / nspath.filename()).generic_string(), sit->second.desc);
                return true;
            }
            // f : f (new)
            else {
                files_.emplace(ndpath_str, sit->second.desc);
                return true;
            }
        } else if (auto sit = dirs_.find(nspath_str); sit != dirs_.end()) {
            // d : f (replace) (no)
            if (auto dit = files_.find(ndpath_str); dit != files_.end()) {
                return false;
            }
            // d : d (new) or d : d
            else {
                absl::flat_hash_map<std::string, FileEntry> files2cp;
                for (const auto& [dpath, entry] : files_) {
                    std::shared_lock guard{ entry.mutex };

                    stdfs::path path = dpath;
                    if (path.parent_path() == ndpath && recursive) {
                        files2cp.emplace((ndpath / path.filename()).generic_string(), entry.desc);
                    } else
                        return false;
                }

                dirs_.emplace(ndpath_str);

                if (!files2cp.empty())
                    files_.merge(files2cp);
                return true;
            }
        }

        return false;
    }

    bool MemoryMount::mv(const std::string& src_rel_path, const std::string& dst_rel_path) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        const stdfs::path nspath     = normalize(src_rel_path);
        const stdfs::path ndpath     = normalize(dst_rel_path);
        std::string       nspath_str = nspath.generic_string();
        std::string       ndpath_str = ndpath.generic_string();

        if (nspath.empty() || ndpath.empty())
            return false;

        if (!exists(ndpath.parent_path().generic_string()) && !exists(nspath.generic_string()))
            return false;

        if (auto sit = files_.find(nspath_str); sit != files_.end()) {
            std::scoped_lock guard{ sit->second.mutex };

            // f : f (replace)
            if (auto dit = files_.find(ndpath_str); dit != files_.end()) {
                std::scoped_lock guard{ dit->second.mutex };

                dit->second.desc      = std::move(sit->second.desc);
                dit->second.desc.path = ndpath;
            }
            // f : d
            else if (auto dit = dirs_.find(ndpath_str); dit != dirs_.end()) {
                std::string dpath     = (stdfs::path{ ndpath } / stdfs::path{ nspath }.filename()).generic_string();
                sit->second.desc.path = dpath;
                files_.emplace(dpath, std::move(sit->second.desc));
            }
            // f : f (new)
            else {
                sit->second.desc.path = ndpath.generic_string();
                files_.emplace(ndpath.generic_string(), std::move(sit->second.desc));
            }

            files_.erase(sit);
            return true;
        } else if (auto sit = dirs_.find(nspath_str); sit != dirs_.end()) {
            // d : d (replace) or d : d (new)
            if (auto dit = dirs_.find(ndpath_str); dit != dirs_.end()) {
                // Move directories
                {
                    std::vector<std::pair<std::string, std::string>> dir_moves;
                    for (const auto& dir : dirs_) {
                        if (is_under(dir, nspath_str)) {
                            std::string suffix = dir.substr(nspath_str.size());
                            dir_moves.emplace_back(dir, ndpath_str + suffix);
                        }
                    }
                    for (const auto& [old_dir, new_dir] : dir_moves) {
                        dirs_.erase(old_dir);
                        if (!dirs_.contains(new_dir))
                            dirs_.insert(new_dir);
                    }
                }

                // Move files
                {
                    struct file_move_descriptor
                    {
                        decltype(files_)::iterator old_file_it;
                        std::string                new_path;
                        FileEntry::Descriptor      desc;
                    };

                    std::vector<file_move_descriptor> file_moves;
                    for (auto it = files_.begin(); it != files_.end(); ++it) {
                        std::lock_guard guard{ it->second.mutex };

                        if (is_under(it->first, nspath_str)) {
                            std::string suffix = it->first.substr(nspath_str.size());
                            file_moves.emplace_back(file_move_descriptor{
                                .old_file_it = it,
                                .new_path    = ndpath_str + suffix,
                                .desc        = std::move(it->second.desc),
                            });
                        }
                    }
                    for (auto& [old_file_it, new_file_path, desc] : file_moves) {
                        files_.erase(old_file_it);
                        desc.path = new_file_path;
                        if (files_.contains(new_file_path)) {
                            std::lock_guard guard{ files_[new_file_path].mutex };
                            files_[new_file_path] = std::move(desc);
                        } else
                            files_[new_file_path] = std::move(desc);
                    }
                }
            }
            // d : f (no)
            else if (auto dit = files_.find(ndpath_str); dit != files_.end()) {
                return false;
            }

            return true;
        }

        return false;
    }

    std::vector<std::string> MemoryMount::list(const std::string& rel_path, const ListOptions opts) const
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

    bool MemoryMount::directory(const std::string& rel_path) const noexcept
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

    bool MemoryMount::empty(const std::string& rel_path) const noexcept
    {
        std::shared_lock lock{ mutex_ };
        const auto       npath = normalize(rel_path);

        if (dirs_.contains(npath)) {
            for (const auto& [path, entry] : files_) {
                if (is_under(path, npath))
                    return false;
            }
            for (const std::string& path : dirs_) {
                if (is_under(path, npath))
                    return false;
            }
        } else if (files_.contains(npath)) {
            const FileEntry& entry = files_.at(npath);
            std::shared_lock guard{ entry.mutex };
            return entry.desc.buffer.empty();
        }
    }
} // namespace codex::fs
