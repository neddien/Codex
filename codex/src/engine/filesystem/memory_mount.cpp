#include "memory_mount.h"

#include "public/file_handle.h"
#include "public/filesystem.h"

namespace codex::fs {
    namespace {
        // Path helpers operating on already-normalized paths (no leading/trailing slashes).
        [[nodiscard]] std::string filename_of(const std::string& p) noexcept
        {
            const auto pos = p.find_last_of('/');
            return pos == std::string::npos ? p : p.substr(pos + 1);
        }
        [[nodiscard]] std::string parent_of(const std::string& p) noexcept
        {
            const auto pos = p.find_last_of('/');
            return pos == std::string::npos ? std::string{} : p.substr(0, pos);
        }
        [[nodiscard]] std::string join_path(const std::string& a, const std::string& b) noexcept
        {
            if (a.empty())
                return b;
            if (b.empty())
                return a;
            return a + '/' + b;
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

        const auto hash = util::crypto::fnv1a(normalize(path));
        return files_.contains(hash) || dirs_.contains(hash);
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

        const stdfs::path nspath = normalize(src_rel_path);
        const stdfs::path ndpath = normalize(dst_rel_path);

        if (nspath.empty() || ndpath.empty())
            return false;

        if (!exists(ndpath.parent_path().generic_string()) && !exists(nspath.generic_string()))
            return false;

        // /a/file.t /b/
        // /a/file.t /a/file2.t (also includes replace)
        // /a/       /b/
        // /a/       /b/c

        if (auto sit = files_.find(nspath); sit != files_.end()) {
            // f : f (replace)
            if (auto dit = files_.find(ndpath); dit != files_.end()) {
                dit->second.buffer     = sit->second.buffer;
                dit->second.last_modif = util::chron::unix_epoch_now();
                return true;
            }
            // f : d
            else if (auto dit = dirs_.find(ndpath); dit != dirs_.end()) {
                files_.emplace((ndpath / nspath.filename()).generic_string(),
                               FileEntry{
                                   .buffer     = sit->second.buffer,
                                   .last_modif = util::chron::unix_epoch_now(),
                               });
                return true;
            }
            // f : f (new)
            else {
                files_.emplace(ndpath.generic_string(), FileEntry{
                                                            .buffer     = sit->second.buffer,
                                                            .last_modif = util::chron::unix_epoch_now(),
                                                        });
                return true;
            }
        } else if (auto sit = dirs_.find(nspath); sit != dirs_.end()) {
            // d : f (replace) (no)
            if (auto dit = files_.find(ndpath); dit != files_.end()) {
                return false;
            }
            // d : d (new) or d : d
            else {
                absl::flat_hash_map<std::string, FileEntry> files2cp;
                for (const auto& [dpath, entry] : files_) {
                    stdfs::path path = dpath;
                    if (path.parent_path() == ndpath && recursive) {
                        files2cp.emplace((ndpath / path.filename()).generic_string(),
                                         FileEntry{
                                             .buffer     = entry.buffer,
                                             .last_modif = entry.last_modif,
                                         });
                    } else
                        return false;
                }

                if (auto dit = dirs_.find(ndpath); dit == dirs_.end())
                    dirs_.emplace(ndpath);

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

        const stdfs::path nspath = normalize(src_rel_path);
        const stdfs::path ndpath = normalize(dst_rel_path);
        const usize       nshash = util::crypto::fnv1a(nspath.generic_string());
        const usize       ndhash = util::crypto::fnv1a(ndpath.generic_string());

        if (nspath.empty() || ndpath.empty())
            return false;

        if (!exists(ndpath.parent_path().generic_string()) && !exists(nspath.generic_string()))
            return false;

        // /a/file.t /b/
        // /a/file.t /a/file2.t (also includes replace)
        // /a/       /b/
        // /a/       /b/c

        if (auto sit = files_.find(nshash); sit != files_.end()) {
            // f : f (replace)
            if (auto dit = files_.find(ndhash); dit != files_.end()) {
                dit->second.buffer     = std::move(sit->second.buffer);
                dit->second.last_modif = util::chron::unix_epoch_now();
                dit->second.read_only  = false;
                return true;
            }
            // f : d/f
            else if (auto dit = dirs_.find(ndhash); dit != dirs_.end()) {
                return true;
            }
            // f : f (new)
            else {
                files_.emplace(util::crypto::fnv1a(ndpath.generic_string()),
                               FileEntry{
                                   .buffer = std::move(sit->second.buffer),
                                   .path   = std::move(sit->second.path),
                               });
            }

            files_.erase(sit);
            return true;
        } else if (auto sit = dirs_.find(nshash); sit != dirs_.end()) {
            // d : d/d
            if (auto dit = dirs_.find(ndhash); dit != dirs_.end()) {
            }
            // d : f (no)
            else if (auto dit = files_.find(ndhash); dit != files_.end()) {
                return false;
            }
            // d : d (new)
            else {
            }
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
