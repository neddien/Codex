#include "vfs.h"

#include "memory_mount.h"
#include "public/filesystem.h"

#include <engine/core/engine.h>

#include <lz4.h>

namespace codex::fs {
    namespace stdfs = std::filesystem;

    namespace {
        bool stream_file(Shared<FileHandle> src, Shared<FileHandle> dst)
        {
            if (not(src && dst))
                return false;

            constexpr auto          buf_len = 1024 * 1024; // 1MiB
            std::array<u8, buf_len> buf;

            src->seek(0);
            dst->seek(0);
            while (true) {
                const usize n = src->read(buf.data(), buf.size());
                if (n == 0)
                    break;

                if (dst->write(buf.data(), n) != n)
                    return false;
            }

            dst->flush();
            return true;
        }
    } // namespace

    VirtualFilesystem::VirtualFilesystem(Shared<IVFSMount> default_mount) noexcept
    {
        root_         = Box<Node>::make();
        root_->dir    = true;
        root_->path   = "/";
        root_->mounts = {};

        if (default_mount) {
            mount(std::move(default_mount), "/");
        } else {
            // Default priority is 0
            mount(Shared<MemoryMount>::make(0), "/");
        }
    }

    bool VirtualFilesystem::mount(Shared<IVFSMount> mount, const std::string& path, const bool mk) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        const auto npath = normalize(path);
        Node*      node  = walk_to(npath);

        if (!node) {
            if (mk) {
                ensure_mount_point_nolock(npath, true);
                node = walk_to(npath);
            } else {
                return false;
            }
        }

        auto pos = std::lower_bound(node->mounts.begin(), node->mounts.end(), mount,
                                    [](const Shared<IVFSMount>& a, const Shared<IVFSMount>& b)
                                    { return a->priority() > b->priority(); });

        node->mounts.insert(pos, mount);

        return true;
    }

    bool VirtualFilesystem::unmount(const std::string& path, Shared<IVFSMount> mount) noexcept
    {
        std::scoped_lock lock{ mutex_ };

        Node* node = walk_to(normalize(path));
        if (!node)
            return false;

        if (!mount) {
            node->mounts.clear();
            return true;
        }

        auto it = std::find_if(node->mounts.begin(), node->mounts.end(),
                               [&mount](const Shared<IVFSMount>& m) { return m.get() == mount.get(); });
        if (it == node->mounts.end())
            return false;

        node->mounts.erase(it);
        return true;
    }

    Shared<FileHandle> VirtualFilesystem::open(const std::string& path, const FileProperties props) noexcept
    {
        const auto npath       = normalize(path);
        auto       components  = util::str::split(npath, '/');
        Node*      mount_node  = root_.get();
        usize      mount_depth = 0;

        {
            std::shared_lock lock{ mutex_ };

            Node* cur   = root_.get();
            usize depth = 0;
            for (const auto& c : components) {
                auto it = cur->children.find(c);
                if (it == cur->children.end())
                    break;

                cur = it->second.get();
                ++depth;

                if (!cur->mounts.empty()) {
                    mount_node  = cur;
                    mount_depth = depth;
                }
            }
        }

        const auto local_path = util::str::join(components.begin() + mount_depth, components.end(), '/');

        for (auto& m : mount_node->mounts) {
            if (auto fh = m->open(local_path, props))
                return fh;
        }

        return nullptr;
    }

    bool VirtualFilesystem::exists(const std::string& path) const noexcept
    {
        std::shared_lock lock{ mutex_ };

        auto  components  = util::str::split(normalize(path), '/');
        auto* cur         = root_.get();
        auto* mount_node  = root_.get();
        usize mount_depth = 0;
        usize depth       = 0;

        for (const auto& c : components) {
            auto it = cur->children.find(c);
            if (it == cur->children.end())
                break;

            cur = it->second.get();
            ++depth;

            if (!cur->mounts.empty()) {
                mount_node  = cur;
                mount_depth = depth;
            }
        }

        if (depth == components.size())
            return true;

        const auto local_path = util::str::join(components.begin() + mount_depth, components.end(), '/');
        for (auto& m : mount_node->mounts) {
            if (m->exists(local_path))
                return true;
        }

        return false;
    }

    bool VirtualFilesystem::mkdir_nolock(const std::string& path, const bool recursive) noexcept
    {
        const auto npath      = normalize(path);
        auto       components = util::str::split(npath, '/');
        auto*      cur        = root_.get();

        Node* mount_node  = cur->mounts.empty() ? nullptr : cur;
        usize mount_depth = 0;
        usize depth       = 0;

        for (auto it = components.begin(); it != components.end(); ++it, ++depth) {
            const auto& c = *it;

            if (auto dir_it = cur->children.find(c); dir_it != cur->children.end()) {
                cur = dir_it->second.get();

                if (!cur->mounts.empty()) {
                    mount_node  = cur;
                    mount_depth = depth + 1;
                }
            }
        }

        if (mount_node) {
            const auto rel_path = util::str::join(components.begin() + mount_depth, components.end(), '/');
            if (!rel_path.empty()) {
                for (auto& m : mount_node->mounts) {
                    if (m->mkdir(rel_path))
                        return true;
                }
            }
        }

        return false;
    }

    bool VirtualFilesystem::ensure_mount_point_nolock(const std::string& path, const bool recursive) noexcept
    {
        const auto npath      = normalize(path);
        auto       components = util::str::split(npath, '/');
        auto*      cur        = root_.get();

        for (auto it = components.begin(); it != components.end(); ++it) {
            const auto& c = *it;

            if (auto dir_it = cur->children.find(c); dir_it != cur->children.end()) {
                cur = dir_it->second.get();
            } else {
                if (std::prev(components.end()) == it || recursive) {
                    Node node{ .path = c, .mounts = {}, .children = {}, .dir = true };
                    auto [ins, _] = cur->children.emplace(c, Box<Node>::make(std::move(node)));
                    cur           = ins->second.get();
                } else
                    return false;
            }
        }

        return true;
    }

    bool VirtualFilesystem::rm_nolock(const std::string& path, const bool recursive) noexcept
    {
        const auto npath      = normalize(path);
        auto       components = util::str::split(npath, '/');
        auto*      cur        = root_.get();

        Node* mount_node  = cur->mounts.empty() ? nullptr : cur;
        usize mount_depth = 0;
        usize depth       = 0;

        for (auto it = components.begin(); it != components.end(); ++it, ++depth) {
            const auto& c = *it;

            if (auto dir_it = cur->children.find(c); dir_it != cur->children.end()) {
                cur = dir_it->second.get();

                if (!cur->mounts.empty()) {
                    mount_node  = cur;
                    mount_depth = depth + 1;
                }
            } else
                break;
        }

        if (mount_node) {
            const auto rel_path = util::str::join(components.begin() + mount_depth, components.end(), '/');
            if (!rel_path.empty()) {
                for (auto& m : mount_node->mounts) {
                    if (m->rm(rel_path))
                        break;
                }
            }
        }

        return true;
    }

    bool VirtualFilesystem::cp_nolock(const std::string& src_path, const std::string& dst_path,
                                      const bool recursive) noexcept
    {
        const auto src_npath      = normalize(src_path);
        const auto dst_npath      = normalize(dst_path);
        auto       src_components = util::str::split(src_npath, '/');
        auto       dst_components = util::str::split(dst_npath, '/');
        auto*      cur            = root_.get();

        Node* src_mount_node  = cur->mounts.empty() ? nullptr : cur;
        usize src_mount_depth = 0;
        usize depth           = 0;

        for (auto it = src_components.begin(); it != src_components.end(); ++it, ++depth) {
            const auto& c = *it;

            if (auto dir_it = cur->children.find(c); dir_it != cur->children.end()) {
                cur = dir_it->second.get();

                if (!cur->mounts.empty()) {
                    src_mount_node  = cur;
                    src_mount_depth = depth + 1;
                }
            } else
                break;
        }

        Node* dst_mount_node  = cur->mounts.empty() ? nullptr : cur;
        depth                 = 0;
        usize dst_mount_depth = 0;
        cur                   = root_.get();

        for (auto it = dst_components.begin(); it != dst_components.end(); ++it, ++depth) {
            const auto& c = *it;

            if (auto dir_it = cur->children.find(c); dir_it != cur->children.end()) {
                cur = dir_it->second.get();

                if (!cur->mounts.empty()) {
                    dst_mount_node  = cur;
                    dst_mount_depth = depth + 1;
                }
            } else
                break;
        }

        IVFSMount* src_mnt{};
        IVFSMount* dst_mnt{};

        if (src_mount_node) {
            for (Shared<IVFSMount>& mnt : src_mount_node->mounts) {
                if (mnt)
                    src_mnt = mnt.get();
            }
        }
        if (dst_mount_node) {
            for (Shared<IVFSMount>& mnt : dst_mount_node->mounts) {
                if (mnt)
                    dst_mnt = mnt.get();
            }
        }

        if (!dst_mnt || !src_mnt)
            return false;

        const auto rel_src_path = util::str::join(src_components.begin() + src_mount_depth, src_components.end(), '/');
        const auto rel_dst_path = util::str::join(dst_components.begin() + dst_mount_depth, dst_components.end(), '/');

        if (src_mnt == dst_mnt) {
            src_mnt->cp(rel_src_path, rel_dst_path, recursive);
            return true;
        }

        // Dir
        if (src_mnt->directory(rel_src_path)) {
            // check if dir empty and recursive == false, then we good
            // otherwise we bad unless recursive == true
            if (src_mnt->empty(rel_src_path)) {
                // dont care about recursive copy anyways
                dst_mnt->mkdir(dst_components.back());
                return true;
            } else if (recursive) {
                std::vector<std::string> dirs =
                    src_mnt->list(rel_src_path, ListOptions::DirsOnly | ListOptions::Recursive);
                std::vector<std::string> files =
                    src_mnt->list(rel_src_path, ListOptions::FilesOnly | ListOptions::Recursive);

                std::sort(dirs.begin(), dirs.end(), [](const std::string& a, const std::string& b)
                          { return std::ranges::count(a, '/') < std::ranges::count(b, '/'); });

                for (const std::string& dir : dirs)
                    dst_mnt->mkdir(dir);

                for (const std::string& file : files) {
                    Shared<FileHandle> sfh = src_mnt->open(file, { FileMode::Read });
                    Shared<FileHandle> dfh =
                        dst_mnt->open(file, { FileMode::Create | FileMode::Trunc | FileMode::Write });
                    if (not stream_file(std::move(sfh), std::move(dfh)))
                        return false;
                }

                return true;
            }
        }
        // File
        else {
            // f : d
            Shared<FileHandle> sfh = src_mnt->open(rel_src_path, { FileMode::Read });
            Shared<FileHandle> dfh;
            if (dst_mnt->directory(rel_dst_path)) {
                dfh = dst_mnt->open(rel_dst_path + "/" + src_components.back(),
                                    { FileMode::Create | FileMode::Trunc | FileMode::Write });
            }
            // f : f (replace or create both cases here)
            else {
                dfh = dst_mnt->open(rel_dst_path, { FileMode::Create | FileMode::Trunc | FileMode::Write });
            }
            return stream_file(std::move(sfh), std::move(dfh));
        }

        return false;
    }

    bool VirtualFilesystem::mv_nolock(const std::string& src_path, const std::string& dst_path) noexcept
    {
        return true;
    }

    bool VirtualFilesystem::mkdir(const std::string& path, const bool recursive) noexcept
    {
        std::scoped_lock lock{ mutex_ };
        return mkdir_nolock(path, recursive);
    }

    bool VirtualFilesystem::ensure_mount_point(const std::string& path, const bool recursive) noexcept
    {
        std::scoped_lock guard{ mutex_ };
        return ensure_mount_point_nolock(path, recursive);
    }

    bool VirtualFilesystem::rm(const std::string& path, const bool recursive) noexcept
    {
        std::scoped_lock lock{ mutex_ };
        return rm_nolock(path, recursive);
    }

    bool VirtualFilesystem::cp(const std::string& src_path, const std::string& dst_path, const bool recursive) noexcept
    {
        std::scoped_lock lock{ mutex_ };
        return cp_nolock(src_path, dst_path, recursive);
    }

    bool VirtualFilesystem::mv(const std::string& src_path, const std::string& dst_path) noexcept
    {
        std::scoped_lock lock{ mutex_ };
        return mv_nolock(src_path, dst_path);
    }

    std::vector<std::string> VirtualFilesystem::list_nolock(const std::string& dir) const noexcept
    {
        auto  components  = util::str::split(normalize(dir), '/');
        auto* cur         = root_.get();
        auto* mount_node  = root_.get();
        usize mount_depth = 0;
        usize depth       = 0;

        for (const auto& c : components) {
            auto it = cur->children.find(c);
            if (it == cur->children.end())
                break;

            cur = it->second.get();
            ++depth;

            if (!cur->mounts.empty()) {
                mount_node  = cur;
                mount_depth = depth;
            }
        }

        std::unordered_set<std::string> result;

        // Explicit trie children at the exact node.
        if (depth == components.size()) {
            for (const auto& [name, _] : cur->children)
                result.insert(name);
        }

        // Virtual contents from the responsible mount
        const auto local_path = util::str::join(components.begin() + mount_depth, components.end(), '/');
        for (auto& m : mount_node->mounts) {
            for (auto& entry : m->list(local_path))
                result.insert(entry);
        }

        return { result.begin(), result.end() };
    }

    bool VirtualFilesystem::is_directory_nolock(const std::string& path) const noexcept
    {
        auto  components  = util::str::split(normalize(path), '/');
        auto* cur         = root_.get();
        auto* mount_node  = root_.get();
        usize mount_depth = 0;
        usize depth       = 0;

        for (const auto& c : components) {
            auto it = cur->children.find(c);
            if (it == cur->children.end())
                break;

            cur = it->second.get();
            ++depth;

            if (!cur->mounts.empty()) {
                mount_node  = cur;
                mount_depth = depth;
            }
        }

        if (depth == components.size())
            return cur->dir;

        const auto local_path = util::str::join(components.begin() + mount_depth, components.end(), '/');
        for (auto& m : mount_node->mounts) {
            if (m->directory(local_path))
                return true;
        }

        return false;
    }

    std::vector<std::string> VirtualFilesystem::list(const std::string& dir, const ListOptions opts) const noexcept
    {
        const bool recursive  = !!(opts & ListOptions::Recursive);
        const bool files_only = !!(opts & ListOptions::FilesOnly);
        const bool dirs_only  = !!(opts & ListOptions::DirsOnly);

        // Build an absolute base that always begins with '/'.
        const auto make_abs = [](const std::string& d) -> std::string
        {
            const auto n = normalize(d);
            return n.empty() ? "/" : '/' + n;
        };
        const auto join_abs = [](const std::string& base, const std::string& name) -> std::string
        { return (base == "/") ? '/' + name : base + '/' + name; };

        std::shared_lock lock{ mutex_ };

        if (!recursive) {
            auto                     entries  = list_nolock(dir);
            const auto               abs_base = make_abs(dir);
            std::vector<std::string> result;
            result.reserve(entries.size());
            for (auto& name : entries) {
                const std::string full = join_abs(abs_base, name);
                if (files_only || dirs_only) {
                    const bool is_dir = is_directory_nolock(full);
                    if ((is_dir && !files_only) || (!is_dir && !dirs_only))
                        result.push_back(full);
                } else {
                    result.push_back(full);
                }
            }
            return result;
        }

        // Recursive iterative DFS
        std::vector<std::string>                        result;
        std::stack<std::pair<std::string, std::string>> stk;
        stk.push({ make_abs(dir), "" });
        while (!stk.empty()) {
            auto [cur_dir, prefix] = std::move(stk.top());
            stk.pop();
            for (auto& name : list_nolock(cur_dir)) {
                const std::string full        = join_abs(cur_dir, name);
                const std::string result_path = prefix.empty() ? name : prefix + '/' + name;
                const bool        is_dir      = is_directory_nolock(full);
                if (is_dir) {
                    if (!files_only)
                        result.push_back(full);
                    stk.push({ full, result_path });
                } else {
                    if (!dirs_only)
                        result.push_back(full);
                }
            }
        }
        return result;
    }

    bool VirtualFilesystem::is_directory(const std::string& path) const noexcept
    {
        std::shared_lock lock{ mutex_ };
        return is_directory_nolock(path);
    }

    cc::task<Shared<FileHandle>> VirtualFilesystem::open_async(std::string path, FileProperties props) noexcept
    {
        co_await Engine::worker_pool();
        co_return open(path, props);
    }

    cc::task<bool> VirtualFilesystem::exists_async(std::string path) const noexcept
    {
        co_await Engine::worker_pool();
        co_return exists(path);
    }

    cc::task<bool> VirtualFilesystem::mkdir_async(std::string path, bool recursive) noexcept
    {
        co_await Engine::worker_pool();
        co_return mkdir(path, recursive);
    }

    cc::task<std::vector<std::string>> VirtualFilesystem::list_async(std::string dir) const noexcept
    {
        co_await Engine::worker_pool();
        co_return list(dir);
    }

    cc::task<bool> VirtualFilesystem::is_directory_async(std::string path) const noexcept
    {
        co_await Engine::worker_pool();
        co_return is_directory(path);
    }

    bool VirtualFilesystem::export_to_pak(Shared<FileHandle> out, const PakProperties props) noexcept
    {
        std::unique_lock lock{ mutex_ };

        const auto is_precompressed = [&](const std::string& path)
        {
            const auto dot = path.rfind('.');
            if (dot == std::string::npos)
                return false;
            std::string ext{ path.substr(dot) };
            for (auto& c : ext)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            for (const auto& e : props.precompressed_exts)
                if (ext == e)
                    return true;
            return false;
        };

        struct entry
        {
            PakEntry           pak_entry;
            usize              offset;
            Shared<FileHandle> handle;
        };

        Node*                                     cur         = root_.get();
        u64                                       offset_idx  = 0;
        u64                                       entry_count = 0;
        u64                                       data_offset = 0;
        u64                                       data_size   = 0;
        std::stack<std::pair<std::string, Node*>> node_st;
        std::vector<entry>                        entries;
        std::unordered_set<std::string>           seen_paths;

        node_st.emplace("", root_.get());

        auto header = PakHeader{
            .version         = { PakVersion[0], PakVersion[1], PakVersion[2] },
            .content_version = { props.version[0], props.version[1], props.version[2] },
            .pak_name        = {},
            .flags           = props.flags,
            .entry_count     = 0,
            .data_offset     = 0,
            .data_size       = 0,
            .chunk_size      = props.chunk_size,
            .last_modified   = util::chron::unix_epoch_now(),
            .reserved        = ~((u64)0),
        };
        std::memcpy(header.pak_name, props.name.c_str(), props.name.size());

        // Phase 1: Compress all the files inside the VFS into LZ4, store their blobs in a temporary directory.
        while (!node_st.empty()) {
            auto [parent_path, cur] = node_st.top();
            node_st.pop();

            std::string cur_path = (parent_path.empty())
                                       ? cur->path
                                       : (parent_path + ((parent_path == "/") ? cur->path : '/' + cur->path));

            // If we're directory then skip i assume since paks don't have the idea of a
            // directory, they just contain compressed files where the path is just the string key
            // for that file.
            for (auto& mount : cur->mounts) {
                for (const auto& list_entry : mount->list("/", ListOptions::FilesOnly | ListOptions::Recursive)) {
                    auto handle = mount->open(list_entry);
                    if (!handle) {
                        error("Cooker: Failed to open file: {} for cooking", list_entry);
                        return false;
                    }

                    const auto full_path = cur_path + '/' + list_entry;
                    const auto rel_path  = std::string_view{ full_path }.substr(1);

                    if (rel_path ==
                        std::string_view{ out->path() }.substr(std::string_view{ out->path() }.find_first_not_of('/')))
                        continue;

                    const std::string rel_str{ rel_path };
                    if (seen_paths.count(rel_str))
                        continue;
                    seen_paths.insert(rel_str);

                    if (rel_path.size() >= sizeof(PakEntry::path)) {
                        error("Cooker: Path too long for PAK entry (max {} chars): {}", sizeof(PakEntry::path) - 1,
                              full_path);
                        return false;
                    }

                    PakEntry entry{};
                    entry.path_hash = util::crypto::djb2_hash(rel_path);
                    std::memcpy(entry.path, rel_path.data(), rel_path.size());
                    entry.path[rel_path.size()] = '\0';
                    entry.data_offset           = offset_idx++;
                    entry.uncompressed_size     = handle->size();
                    entry.last_modified         = header.last_modified;

                    if ((props.flags & PakFlags::Compressed) && handle->size() >= props.compression_threshold &&
                        !is_precompressed(list_entry)) {
                        entry.flags = PakFlags::Compressed | PakFlags::CompressionTypeLZ4;
                    }

                    entries.emplace_back(entry, 0, std::move(handle));
                }
            }

            for (auto& [_, node] : cur->children) {
                if (node) {
                    node_st.emplace(cur_path, node.get());
                }
            }
        }

        lock.unlock(); // release before I/O-heavy phases 2 and 3

        // Phase 2: Update metdata and write it to our PAK file
        // Update header
        header.entry_count = entries.size();
        header.data_offset = sizeof(header) + sizeof(PakEntry) * header.entry_count;

        // Write header
        out->write(&header, sizeof(header));
        for (auto& [entry, offset, _] : entries) {
            offset = out->tell();
            out->write(&entry, sizeof(entry));
        }

        // Phase 3: Compression and write to PAK
        for (usize i = 0; i < entries.size(); ++i) {
            auto& [entry, offset, handle] = entries[i];

            handle->seek(0);

            const auto entry_data_offset = out->tell();

            usize bytes_read  = 0;
            u32   running_crc = 0xFFFFFFFFu;

            if (entry.flags & (PakFlags::Compressed | PakFlags::CompressionTypeLZ4)) {
                usize            compressed_size = 0;
                std::vector<u8>  chunk(header.chunk_size);
                std::vector<u8>  dst(LZ4_compressBound(header.chunk_size));
                std::vector<u64> chunk_offset_table(entry.uncompressed_size / header.chunk_size + 1);
                usize            chunk_idx = 0;

                const auto chunk_table_offset      = out->tell();
                const auto chunk_offset_table_size = chunk_offset_table.size() * sizeof(chunk_offset_table[0]);

                // Allocate space for the chunk offset table
                out->seek(out->tell() + chunk_offset_table_size);

                while ((bytes_read = handle->read(chunk.data(), header.chunk_size)) > 0) {
                    running_crc = util::crypto::crc32_update(chunk.data(), bytes_read, running_crc);

                    const i32 comp_size = LZ4_compress_default(
                        reinterpret_cast<const char*>(chunk.data()), reinterpret_cast<char*>(dst.data()),
                        static_cast<i32>(bytes_read), static_cast<i32>(dst.size()));

                    chunk_offset_table[chunk_idx++] = out->tell();

                    const u32 comp_u32 = static_cast<u32>(comp_size);
                    out->write(&comp_u32, sizeof(comp_u32));
                    out->write(dst.data(), comp_size);
                    compressed_size += sizeof(comp_u32) + comp_size;
                }

                entry.compressed_size = compressed_size;
                entry.chunk_count     = chunk_idx;

                out->write_at(chunk_offset_table.data(), chunk_offset_table_size, chunk_table_offset);
            } else {
                std::vector<u8> buffer(header.chunk_size);
                while ((bytes_read = handle->read(buffer.data(), buffer.size())) > 0) {
                    running_crc = util::crypto::crc32_update(buffer.data(), bytes_read, running_crc);
                    out->write(buffer.data(), bytes_read);
                }
            }

            entry.crc = util::crypto::crc32_finalize(running_crc);

            // Update the PakEntry
            entry.data_offset = entry_data_offset;
            out->write_at(&entry, sizeof(entry), offset);
        }

        return true;
    }

    VirtualFilesystem::Node* VirtualFilesystem::walk_to(const std::string& path, Node** const previous_node) noexcept
    {
        const auto npath = normalize(path);
        Node*      cur   = root_.get();

        for (const auto& c : util::str::split(npath, '/')) {
            if (auto it = cur->children.find(c); it != cur->children.end()) {
                cur = it->second.get();
            } else {
                if (previous_node)
                    *previous_node = cur;

                return nullptr;
            }
        }

        return cur;
    }
} // namespace codex::fs
