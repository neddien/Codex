#include "pak_mount.h"

#include <lz4.h>

namespace codex::fs {
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

    class PakFileHandle : public FileHandle
    {
    public:
        PakFileHandle(const std::string_view path, Shared<FileHandle> handle, const PakEntry entry,
                      Shared<PakMount> owner, const FileProperties props = FileProperties{})
            : path_{ normalize(std::string{ path }) }
            , entry_{ entry }
            , props_{ props }
            , cursor_{ 0 }
            , mutex_{}
            , owner_{ std::move(owner) }
            , handle_{ std::move(handle) }
        {
            if (entry_.flags & PakFlags::Compressed) {
                chunk_offsets_.resize(entry_.chunk_count);
                handle_->read_at(chunk_offsets_.data(), entry_.chunk_count * sizeof(chunk_offsets_[0]),
                                 entry_.data_offset);
                comp_buf_.resize(static_cast<usize>(LZ4_COMPRESSBOUND(static_cast<i32>(owner_->chunk_size()))));
            }
        }

    public:
        usize read(void* dest, const usize len) noexcept override
        {
            std::scoped_lock handle_lock{ mutex_ };
            const usize      result = read_impl(dest, len, cursor_);
            cursor_ += result;
            return result;
        }
        usize write(const void* src, const usize len) noexcept override
        {
            // Paks are read-only.
            return 0;
        }
        usize read_at(void* dest, const usize len, const usize offset) noexcept override
        {
            std::scoped_lock handle_lock{ mutex_ };
            return read_impl(dest, len, offset);
        }

        usize write_at(const void* src, const usize len, const usize offset) noexcept override
        {
            // Paks are read-only.
            return 0;
        }

        void seek(const usize offset) noexcept override
        {
            std::scoped_lock handle_lock{ mutex_ };
            cursor_ = offset; // logical offset within this file, not physical PAK offset
        }
        usize tell() const noexcept override
        {
            std::shared_lock handle_lock{ mutex_ };
            return cursor_;
        }
        usize size() const noexcept override
        {
            std::shared_lock lock{ mutex_ };
            return entry_.uncompressed_size;
        }
        std::string path() const noexcept override
        {
            std::shared_lock handle_lock{ mutex_ };
            return path_;
        }
        u64 last_modified() const noexcept override { return entry_.last_modified; }

    private:
        usize read_impl(void* dest, const usize len, const usize offset) noexcept
        {
            if (!(props_.mode & FileMode::Read) || offset >= entry_.uncompressed_size)
                return 0;

            const usize read_len = std::min(len, entry_.uncompressed_size - offset);

            if (entry_.flags & PakFlags::Compressed) {
                const usize chunk_size   = owner_->chunk_size();
                const usize chunk_idx_lo = offset / chunk_size;
                const usize chunk_idx_hi = (offset + read_len - 1) / chunk_size;
                const usize num_chunks   = chunk_idx_hi - chunk_idx_lo + 1;

                std::vector<u8> decomp_bufs(num_chunks * chunk_size);
                u8*             decomp_ptr = decomp_bufs.data();
                for (usize i = chunk_idx_lo; i <= chunk_idx_hi; ++i) {
                    u32 comp_size = 0;
                    handle_->read_at(&comp_size, sizeof(comp_size), chunk_offsets_[i]);

                    handle_->read_at(comp_buf_.data(), comp_size, chunk_offsets_[i] + sizeof(comp_size));

                    const i32 decomp_len = LZ4_decompress_safe(
                        reinterpret_cast<const char*>(comp_buf_.data()), reinterpret_cast<char*>(decomp_ptr),
                        static_cast<i32>(comp_size), static_cast<i32>(chunk_size));
                    decomp_ptr += decomp_len;
                }

                std::memcpy(dest, decomp_bufs.data() + (offset % chunk_size), read_len);
            } else {
                handle_->read_at(dest, read_len, entry_.data_offset + offset);
            }

            return read_len;
        }

    private:
        mutable std::shared_mutex mutex_;
        std::string               path_;
        FileProperties            props_;
        usize                     cursor_;
        PakEntry                  entry_;
        std::vector<u64>          chunk_offsets_; // absolute PAK offsets to each chunk's [comp_size][data]
        std::vector<u8>           comp_buf_;      // pre-allocated decomp scratch buffer
        Shared<PakMount>          owner_;
        Shared<FileHandle>        handle_;
    };

    PakMount::PakMount(Shared<FileHandle> handle, const i32 priority)
        : handle_{ std::move(handle) }
        , priority_{ priority }
    {
        if (handle_) {
            handle_->read(&header_, sizeof(header_));

            if (std::memcmp(header_.magic, "CXPAKFILE", 10) != 0)
                throw InvalidOperationException("Invalid PAK file: magic bytes mismatch");

            entries_.resize(header_.entry_count);
            for (usize i = 0; i < header_.entry_count; ++i) {
                handle_->read(&entries_[i], sizeof(entries_[i]));
            }

            // Sort entries by path_hash so open()/exists() can use cache-friendly binary search
            // instead of a hash map (avoids bucket pointer chasing and heap-string comparisons).
            std::sort(entries_.begin(), entries_.end(),
                      [](const PakEntry& a, const PakEntry& b) { return a.path_hash < b.path_hash; });

            // Build a sorted directory list for O(log N) is_directory()/exists().
            for (const auto& e : entries_) {
                std::string_view sv{ e.path };
                usize            pos = 0;
                while ((pos = sv.find('/', pos)) != std::string_view::npos) {
                    dirs_.emplace_back(sv.substr(0, pos++));
                }
            }

            std::sort(dirs_.begin(), dirs_.end());
            dirs_.erase(std::unique(dirs_.begin(), dirs_.end()), dirs_.end());

            // Reject tampered PAKs unless the Insecure flag explicitly opts out.
            const auto corrupted = verify_integrity();
            if (!corrupted.empty())
                throw InvalidOperationException(
                    "PAK integrity check failed: " + std::to_string(corrupted.size()) +
                    (corrupted.size() == 1 ? " corrupt entry detected" : " corrupt entries detected"));
        } else {
            throw InvalidOperationException("File handle for the PAK is null");
        }
    }

    bool PakMount::exists(const std::string& path) const noexcept
    {
        const auto npath = normalize(path);
        if (npath.empty())
            return true;

        // Check files: binary search by hash, then verify string to handle collisions.
        const u64 hash = util::crypto::fnv1a(npath);
        auto      it   = std::lower_bound(entries_.begin(), entries_.end(), hash,
                                          [](const PakEntry& e, u64 h) { return e.path_hash < h; });
        while (it != entries_.end() && it->path_hash == hash) {
            if (std::string_view{ it->path } == npath)
                return true;
            ++it;
        }

        // Check implied directories.
        auto dit = std::lower_bound(dirs_.begin(), dirs_.end(), npath);
        return dit != dirs_.end() && *dit == npath;
    }

    Shared<FileHandle> PakMount::open(const std::string& path, const FileProperties props) noexcept
    {
        const auto npath = normalize(path);
        const u64  hash  = util::crypto::fnv1a(npath);
        auto       it    = std::lower_bound(entries_.begin(), entries_.end(), hash,
                                            [](const PakEntry& e, u64 h) { return e.path_hash < h; });
        while (it != entries_.end() && it->path_hash == hash) {
            if (std::string_view{ it->path } == npath) {
                if (props.mode == FileMode::Read)
                    return Shared<PakFileHandle>::make(npath, handle_, *it, shared_from_this(), props);
                return nullptr;
            }
            ++it;
        }
        return nullptr;
    }

    i32 PakMount::priority() const noexcept
    {
        return priority_;
    }

    bool PakMount::mkdir(const std::string& rel_path) noexcept
    {
        return false;
    }

    std::vector<std::string> PakMount::list(const std::string& rel_path, const ListOptions opts) const
    {
        const auto npath      = normalize(rel_path);
        const auto prefix     = npath.empty() ? std::string{} : npath + '/';
        const bool recursive  = opts & ListOptions::Recursive;
        const bool files_only = opts & ListOptions::FilesOnly;
        const bool dirs_only  = opts & ListOptions::DirsOnly;

        absl::flat_hash_set<std::string> result;

        for (const auto& entry : entries_) {
            const std::string_view ep{ entry.path };

            if (!prefix.empty() && !ep.starts_with(prefix))
                continue;

            const auto rest  = ep.substr(prefix.size());
            const auto slash = rest.find('/');

            if (slash == std::string_view::npos) {
                // Direct file child of this directory.
                if (!dirs_only)
                    result.emplace(rest);
            } else if (recursive) {
                // Add the file at its full relative path.
                if (!dirs_only)
                    result.emplace(rest);
                // Add every intermediate directory component.
                if (!files_only) {
                    auto pos = rest.find('/');
                    while (pos != std::string_view::npos) {
                        result.emplace(rest.substr(0, pos));
                        pos = rest.find('/', pos + 1);
                    }
                }
            } else {
                // Non-recursive: only the immediate child directory name.
                if (!files_only)
                    result.emplace(rest.substr(0, slash));
            }
        }

        return { result.begin(), result.end() };
    }

    bool PakMount::directory(const std::string& rel_path) const noexcept
    {
        const auto npath = normalize(rel_path);
        if (npath.empty())
            return true;
        auto it = std::lower_bound(dirs_.begin(), dirs_.end(), npath);
        return it != dirs_.end() && *it == npath;
    }

    bool PakMount::empty(const std::string& rel_path) const noexcept
    {
        const auto  npath      = normalize(rel_path);
        const usize npath_hash = util::crypto::fnv1a(npath);
        if (std::binary_search(dirs_.begin(), dirs_.end(), rel_path)) {
            for (const std::string& dir : dirs_) {
                if (is_under(dir, npath))
                    return false;
            }
            for (const PakEntry& entry : entries_) {
                if (is_under(entry.path, npath))
                    return false;
            }
            return true;
        } else if (auto it = std::lower_bound(entries_.begin(), entries_.end(), npath_hash,
                                              [](const auto& e, usize hash) { return e.path_hash < hash; });
                   it != entries_.end() && it->path_hash == npath_hash) {
            return it->uncompressed_size == 0;
        }

        return true;
    }

    u64 PakMount::chunk_size() const noexcept
    {
        return header_.chunk_size;
    }

    std::string PakMount::mount_src() const noexcept
    {
        return (handle_) ? handle_->path() : "";
    }

    std::vector<std::string> PakMount::verify_integrity() const
    {
        if (header_.flags & PakFlags::Insecure)
            return {};

        std::vector<std::string> failed;
        const usize              chunk_size = header_.chunk_size;
        std::vector<u8>          buf(chunk_size);
        std::vector<u8>          comp_buf(static_cast<usize>(LZ4_COMPRESSBOUND(static_cast<i32>(chunk_size))));

        for (const auto& entry : entries_) {
            u32   running   = 0xFFFFFFFFu;
            usize remaining = entry.uncompressed_size;
            bool  ok        = true;

            if (entry.flags & PakFlags::Compressed) {
                // Read the chunk offset table, then decompress each chunk and stream CRC.
                std::vector<u64> offsets(entry.chunk_count);
                handle_->read_at(offsets.data(), entry.chunk_count * sizeof(u64), entry.data_offset);

                for (usize i = 0; i < entry.chunk_count && ok; ++i) {
                    u32 comp_size = 0;
                    handle_->read_at(&comp_size, sizeof(comp_size), offsets[i]);
                    handle_->read_at(comp_buf.data(), comp_size, offsets[i] + sizeof(comp_size));

                    const usize this_chunk = std::min(remaining, chunk_size);
                    const i32   decomp_len = LZ4_decompress_safe(
                        reinterpret_cast<const char*>(comp_buf.data()), reinterpret_cast<char*>(buf.data()),
                        static_cast<i32>(comp_size), static_cast<i32>(chunk_size));

                    if (decomp_len < 0) {
                        ok = false;
                        break;
                    }

                    running = util::crypto::crc32_update(buf.data(), this_chunk, running);
                    remaining -= this_chunk;
                }
            } else {
                usize offset = entry.data_offset;
                while (remaining > 0) {
                    const usize to_read = std::min(remaining, chunk_size);
                    handle_->read_at(buf.data(), to_read, offset);
                    running = util::crypto::crc32_update(buf.data(), to_read, running);
                    offset += to_read;
                    remaining -= to_read;
                }
            }

            if (!ok || util::crypto::crc32_finalize(running) != entry.crc)
                failed.emplace_back(entry.path);
        }

        return failed;
    }
} // namespace codex::fs
