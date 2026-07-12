#include "public/asset_registry.h"

#include <engine/core/engine.h>
#include <engine/core/public/binary_archive.h>
#include <engine/core/public/serialization_manager.h>
#include <engine/filesystem/vfs.h>

namespace codex {
    using namespace codex::cc;
    using namespace codex::fs;
    namespace stdfs = std::filesystem;

    namespace {
        // TODO: These utility functions need to be consolidated.
        stdfs::path remove_prefix(const stdfs::path& path, const stdfs::path& prefix)
        {
            auto [p, pre] = std::mismatch(path.begin(), path.end(), prefix.begin(), prefix.end());

            if (pre != prefix.end())
                return path; // prefix doesn't match

            stdfs::path result;
            for (; p != path.end(); ++p)
                result /= *p;

            return result;
        }
    } // namespace

    static u32 checksum_file(FileHandle& handle) noexcept

    {
        constexpr usize chunk_size = 1024 * 1024; // 1MiB
        std::vector<u8> buf(chunk_size);
        u32             crc = 0xFFFFFFFFu;
        usize           n   = 0;

        while ((n = handle.read(buf.data(), chunk_size)) > 0) {
            for (usize i = 0; i < n; ++i) {
                crc ^= buf[i];
                for (int k = 0; k < 8; ++k)
                    crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
            }
        }

        return ~crc;
    };

    AssetRegistry::AssetRegistry(fs::VirtualFilesystem& vfs, std::string_view root_path) noexcept
        : vfs_{ vfs }
        , root_path_{ root_path }
    {
    }

    AssetRegistry::~AssetRegistry() noexcept
    {
        write_meta_files();
        log(Info, "Disposed");
    }

    void AssetRegistry::move_asset(const AssetPath& path, const std::string& new_path)
    {
        std::scoped_lock guard{ mutex_ };
    }

    void AssetRegistry::repath(const AssetPath& path, const std::string& new_path)
    {
        AssetMetadata* meta = asset_metadata(path.uuid());
        assert(meta);

        {
            mutex_.lock_shared();
            auto path_it = path_to_meta_.find(util::crypto::fnv1a(path.path()));
            auto uuid_it = uuid_to_meta_.find(path.uuid());
            mutex_.unlock_shared();
            assert(path_it != path_to_meta_.end());
            assert(uuid_it != uuid_to_meta_.end());

            mutex_.lock();
            path_to_meta_.erase(path_it);
            uuid_to_meta_.erase(uuid_it);
            mutex_.unlock();
        }

        {
            std::scoped_lock guard{ mutex_ };
            meta->path                                            = AssetPath{ new_path, path.uuid() };
            meta->storage_path                                    = new_path;
            meta->dirty                                           = true;
            path_to_meta_[util::crypto::fnv1a(meta->path.path())] = meta;
            uuid_to_meta_[meta->path.uuid()]                      = meta;
        }

        write_meta_files();
    }

    task<void> AssetRegistry::scan_async(const std::string path)
    {
        if (!vfs_.exists(path)) {
            log(Error, "{}: no such file or directory", path);
            co_return;
        }

        if (!vfs_.is_directory(path)) {
            log(Error, "{}: not a directory", path);
            co_return;
        }

        {
            std::scoped_lock guard{ mutex_ };

            metas_.clear();
            orphan_metas_.clear();
            path_to_meta_.clear();
            uuid_to_meta_.clear();
        }

        std::vector<std::string> list = vfs_.list(path, ListOptions::FilesOnly | ListOptions::Recursive);

        // Scan for metadata files first
        std::vector<task<void>> meta_tasks;
        for (stdfs::path path : list) {
            if (path.extension() == ".cxmeta")
                meta_tasks.push_back(resolve_meta(path));
        }

        // TODO: task_group<void> group; co_await group; // Wait for all the tasks inside the group
        for (auto& task : meta_tasks)
            co_await task;

        // Now scan for the assets themselves
        std::vector<task<void>> asset_tasks;
        for (stdfs::path path : list) {
            if (path.extension() != ".cxmeta")
                asset_tasks.push_back(resolve_asset(path));
        }

        // TODO: task_group<void> group; co_await group; // Wait for all the tasks inside the group
        for (auto& task : asset_tasks)
            co_await task;

        // Delete all orphaned metas
        {
            std::scoped_lock guard{ mutex_ };
            orphan_metas_.clear();
        }

        write_meta_files();
    }

    AssetMetadata* AssetRegistry::asset_metadata(const UUID uuid) noexcept
    {
        std::scoped_lock guard{ mutex_ };

        if (auto it = uuid_to_meta_.find(uuid); it != uuid_to_meta_.end())
            return it->second;
        return nullptr;
    }

    AssetMetadata* AssetRegistry::asset_metadata(const std::string& path) noexcept
    {
        std::shared_lock guard{ mutex_ };

        if (auto it = path_to_meta_.find(util::crypto::fnv1a(path)); it != path_to_meta_.end())
            return it->second;
        else
            return nullptr;
    }

    task<void> AssetRegistry::resolve_meta(const stdfs::path path) noexcept
    {
        auto executor = Engine::worker_pool();
        if (executor.thread_pool().available_concurrency() > 0) {
            co_await executor;
        }

        stdfs::path   root_path = root_path_;
        stdfs::path   rel_path  = remove_prefix(path, root_path_);
        AssetMetadata metadata{};
        metadata.path         = AssetPath{ rel_path.generic_string() };
        metadata.storage_path = rel_path.generic_string();
        metadata.dirty        = true;

        // It's an asset meta
        auto fh = vfs_.open(path.generic_string(), { FileMode::Read });
        if (!fh) {
            log(Error, "Failed to open meta file: {}", path.generic_string());
            co_return;
        }

        std::string jastr;
        jastr.resize(fh->size());
        fh->read(jastr.data(), jastr.size());

        try {
            SerializationManager::from_json(metadata, jastr);

            stdfs::path meta_file_path = root_path / metadata.storage_path;
            if (vfs_.exists(meta_file_path.generic_string())) {
                // TODO: This case is wrong
                // path is pointing a cxmeta file
                // while meta_file_path points to the actual asset's path
                // these are always not going to match obviously lol
                // But we do probably need a check if a metafile has been moved outside of codex.
                /*
                if (path != meta_file_path) {
                    // Somehow, the meta file has been moved outside of Codex.
                    // The metafile however is supposed to sit with the asset itself, on the same directory.
                    // Do we move?
                    assert(false);
                }
                */

                // We got a meta that is valid, we return it.
                if (AssetMetadata* m = append_meta(std::move(metadata)); m) {
                    log(Debug, "valid meta has been pushed to the registry: {}, {}", m->path.path(), m->path.uuid());
                }
            } else {
                // Asset file does not exist which means:
                // A: it was deleted outside of Codex
                // B: it was moved
                // C: it was renamed

                // To check if it was moved/renamed, we're gonna rely on:
                // * type
                // * checksum
                // * size
                // If these align to a alleged 'new' asset then this is our moved/renamed asset most likely
                // For this to happen, this meta needs to be orphaned.
                // If an orphaned meta is resolved, then it is simply popped from the orphan_metas_ vector.
                // If the scan finishes and there're still orphaned metas then we'll:
                // * remove from filesystem
                // * clean from vector

                // Orphan the asset meta.
                std::scoped_lock guard{ mutex_ };

                trace("orphaned meta: {}", metadata.path);
                orphan_metas_.push_back(Box<AssetMetadata>::make(std::move(metadata)));

                // Delete orphaned meta from disk
                if (vfs_.rm(path))
                    log(Debug, "Orphan meta deleted: {}", path.generic_string());
                else
                    log(Debug, "Failed to delete orphaned meta: {}", path.generic_string());
            }
        }
        catch (const CodexException& ex) {
            log(Error, "Failed to parse meta file: {}: ", path.generic_string(), ex.what());
            co_return;
        }
    }

    task<void> AssetRegistry::resolve_asset(const stdfs::path path) noexcept
    {
        auto executor = Engine::worker_pool();
        if (executor.thread_pool().available_concurrency() > 0) {
            co_await executor;
        }

        stdfs::path   root_path = root_path_;
        stdfs::path   rel_path  = remove_prefix(path, root_path_);
        AssetMetadata metadata{};
        metadata.path         = AssetPath{ rel_path.generic_string() };
        metadata.storage_path = rel_path.generic_string();
        metadata.dirty        = true;

        const auto ext = path.extension();
        // It's a possible asset
        auto loader = AssetManager::loader_by_ext(ext);
        if (!loader) {
            log(Error, "no loader associated with extension: {}", ext.generic_string());
            co_return;
        }

        trace("possible asset: {}", path.generic_string());

        metadata.type = loader->asset_type_name();

        if (loader->asset_type_id() != typeid(void)) {
            auto handle = vfs_.open(path.generic_string(), { FileMode::Read });
            if (handle) {
                metadata.checksum      = checksum_file(*handle);
                metadata.size          = handle->size();
                metadata.last_modified = handle->last_modified();
            } else {
                log(Error, "handle for file: {} is invalid", path.generic_string());
                co_return;
            }
        } else {
            metadata.null_asset = true;
            log(Info, "Null asset: {}", path.generic_string());
        }

        // Do we have a meta for this guy? Try to find it and if we do update the current meta, just in case.
        {
            std::scoped_lock guard{ mutex_ };

            for (auto& e : metas_) {
                assert(e);

                if (e->path.path() == rel_path.generic_string()) {
                    e->checksum      = metadata.checksum;
                    e->size          = metadata.size;
                    e->dirty         = true;
                    e->last_modified = metadata.last_modified;
                    e->storage_path  = metadata.storage_path;
                    trace("meta already exists for asset: {}", path.generic_string());
                    co_return;
                }
            }
        }
        trace("maybe orphaned meta for asset: {}", path.generic_string());

        // If we don't have meta then the meta might be orphaned, basically the asset was either renamed or moved.
        {
            // Thought about copying orphan_metas_ into a temporary instead of locking the mutex but
            // doing that would probably be slower since its a vec<box<Metadata>>
            std::scoped_lock guard{ mutex_ };
            for (auto it = orphan_metas_.begin(); it != orphan_metas_.end(); ++it) {
                auto& meta = *it;

                // Asset was moved, update the orphaned meta to point to the new path and pop it from orphaned metas
                // and add it back to the registry.
                if (meta->type == metadata.type && meta->checksum == metadata.checksum && meta->size == metadata.size) {
                    meta->path = AssetPath{ remove_prefix(path, root_path).generic_string(), meta->path.uuid() };
                    meta->storage_path = metadata.storage_path;

                    {
                        trace("orphaned asset {} has been resolved", path.generic_string());
                        meta->dirty = true;
                        append_meta_nolock(std::move(*meta));
                        orphan_metas_.erase(it);
                    }
                    co_return;
                }
            }
        }

        // In this case it has to be a new asset no?
        // Create a new metadata
        log(Debug, "new asset registration for: {}", path.generic_string());
        metadata.import_settings = loader->default_import_settings();

        if (AssetMetadata* m = append_meta(std::move(metadata)); m) {
            log(Debug, "valid meta has been pushed to the registry: {}, {}", m->path.path(), m->path.uuid());
        }
    }

    void AssetRegistry::write_meta_files() const noexcept
    {
        std::shared_lock guard{ mutex_ };

        // TODO: Threaded Parallel range based loop
        for (auto& e : metas_) {
            auto path = root_path_ / stdfs::path{ e->storage_path };
            path.replace_extension(path.extension().generic_string() + ".cxmeta");

            // We don't write metadatas for null assets on the disk.
            if (e->dirty && !e->null_asset) {
                if (auto fh =
                        vfs_.open(path.generic_string(), { FileMode::Create | FileMode::Trunc | FileMode::Write });
                    fh) {
                    auto jastr = SerializationManager::to_json(*e) + '\n';
                    fh->write(jastr.data(), jastr.size());
                    e->dirty = false;
                    log(Debug, "Updating meta on disk: {}", path.generic_string());
                } else {
                    log(Error, "failed to open: {} for writing", path.generic_string());
                }
            } else {
                log(Debug, "Asset meta not dirty, not updating {}", e->path.path());
            }
        }
    }

    AssetMetadata* AssetRegistry::append_meta_nolock(AssetMetadata meta) noexcept
    {
        const usize path_hash = util::crypto::fnv1a(meta.path.path());
        const UUID  uuid      = meta.path.uuid();

        if (path_to_meta_.contains(path_hash)) {
            log(Error, "Duplicate asset metadata path: {}", meta.path);
            return nullptr;
        }

        if (uuid_to_meta_.contains(uuid)) {
            log(Error, "Duplicate asset UUID: {}", meta.path);
            return nullptr;
        }

        auto           metabox = Box<AssetMetadata>::make(std::move(meta));
        AssetMetadata* m       = metabox.get();

        path_to_meta_.emplace(path_hash, m);
        uuid_to_meta_.emplace(uuid, m);
        metas_.push_back(std::move(metabox));

        return m;
    }

    AssetMetadata* AssetRegistry::append_meta(AssetMetadata meta) noexcept
    {
        std::scoped_lock guard{ mutex_ };
        return append_meta_nolock(std::move(meta));
    }

    cc::task<void> AssetRegistry::write_manifest_async(const std::string vfs_path) const noexcept
    {
        auto executor = Engine::worker_pool();
        if (executor.thread_pool().available_concurrency() > 0) {
            co_await executor;
        }

        std::shared_lock guard{ mutex_ };

        BinaryArchiveBackend binsd;
        Archive              ar{ binsd };

        // TODO: Threaded Parallel range based loop
        // Drop the const here cause we're only serializing.
        usize meta_count = std::accumulate(metas_.begin(), metas_.end(), 0, [](u64 acc, const Box<AssetMetadata>& meta)
                                           { return (meta and not meta->null_asset) ? ++acc : acc; });

        binsd.begin_array("metadatas", meta_count);
        for (Box<AssetMetadata>& e : const_cast<AssetRegistry*>(this)->metas_) {
            if (!e->null_asset) {
                auto cpy = Box<AssetMetadata>::make(*e);

                // Storage path for exported assets does not match logical path (AssetMetadata::path)
                // Because all assets are stored under a single folder and are renamed to their UUIDs
                cpy->storage_path = "data/" + e->path.uuid().to_string();
                cpy->archive(ar);
            }
        }
        binsd.end_array();

        std::vector<u8> buffer = binsd.take_buffer();
        auto            fh = vfs_.open(vfs_path, { fs::FileMode::Create | fs::FileMode::Write | fs::FileMode::Trunc });
        if (fh)
            fh->write(buffer.data(), buffer.size());
        // TODO: Error handling ?
    }

    cc::task<void> AssetRegistry::export_assets_async(const std::string vfs_path) const noexcept
    {
        auto executor = Engine::worker_pool();
        if (executor.thread_pool().available_concurrency() > 0) {
            co_await executor;
        }

        std::shared_lock guard{ mutex_ };

        trace("trace");

        if (!vfs_.exists(vfs_path)) {
            if (!vfs_.mkdir(vfs_path)) {
                log(Error, "{}: failed to create directory", vfs_path);
                co_return;
            }
        } else {
            if (!vfs_.is_directory(vfs_path)) {
                log(Error, "{}: is a file, not a directory", vfs_path);
                co_return;
            }
        }

        trace("trace");

        for (const Box<AssetMetadata>& e : metas_) {
            // TODO: You should probably do asset_root + e->path.path() when switching away from /editor
            if (!e->null_asset) {
                const std::string source = stdfs::path{ root_path_ } / e->storage_path;
                if (!vfs_.cp(source, stdfs::path{ vfs_path } / e->path.uuid().to_string())) {
                    // TODO: I think you're supposed to throw an exception here instead of just returning
                    log(Error, "Failed to copy asset file: {} for asset cooking", e->path);
                    co_return;
                }
            }
        }
    }

    void AssetRegistry::from_manifest(const std::string& path) noexcept
    {
        std::scoped_lock guard{ mutex_ };

        if (!vfs_.exists(path)) {
            if (vfs_.is_directory(path)) {
                log(Error, "{}: is a directory not a manifest file");
            }
        }

        if (auto fh = vfs_.open(path, { fs::FileMode::Read }); fh) {
            std::vector<u8> buf(fh->size());

            fh->read(buf.data(), buf.size());

            BinaryArchiveBackend binsd{ buf };
            Archive              ar{ binsd };

            usize meta_count;
            binsd.begin_array("metadatas", meta_count);
            for (usize i = 0; i < meta_count; ++i) {
                AssetMetadata meta;
                meta.archive(ar);
                append_meta_nolock(std::move(meta));
            }
            binsd.end_array();

            log(Info, "Asset registry loaded from manifest");
        } else {
            log(Error, "Failed to create registry from manifest file: {}", path);
        }
    }

    void AssetMetadata::archive(Archive& ar)
    {
        path.archive(ar);

        ar("storage_path", storage_path);
        ar("type", type);
        ar("checksum", checksum);
        ar("size", size);
        ar("last_modified", last_modified);
        ar("dependencies", dependencies);

        // import_settings is polymorphic, so it is presence-gated and dispatched by asset type.
        const bool present = ar.backend().optional("import_settings", ar.saving() && import_settings != nullptr);
        if (present) {
            if (ar.loading()) {
                auto loader = AssetManager::loader_by_type(type);
                assert(loader);
                import_settings = loader->default_import_settings();
            }
            if (import_settings) {
                ar.backend().begin_object("import_settings");
                import_settings->archive(ar);
                ar.backend().end_object();
            }
        }
    }
} // namespace codex
