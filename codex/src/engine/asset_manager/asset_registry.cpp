#include "public/asset_registry.h"

#include <engine/core/engine.h>
#include <engine/core/public/serialization_manager.h>
#include <engine/filesystem/vfs.h>

namespace codex {
    using namespace codex::cc;
    using namespace codex::fs;
    namespace stdfs = std::filesystem;

    static u32 checksum_file(FileHandle& handle) noexcept

    {
        constexpr usize chunk_size = 1024 * 1024; // 1024KiB
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

    AssetRegistry::AssetRegistry(fs::VirtualFilesystem& vfs) noexcept
        : vfs_{ vfs }
    {
    }

    AssetRegistry::~AssetRegistry() noexcept
    {
        write_meta_files();
        log(Info, "Diposed");
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
            meta->path              = AssetPath{ new_path, path.uuid() };
            meta->dirty             = true;
            auto [pit, pdid_insert] = path_to_meta_.insert_or_assign(util::crypto::fnv1a(meta->path.path()), meta);
            auto [uit, udid_insert] = uuid_to_meta_.insert_or_assign(meta->path.uuid(), meta);
            assert(pdid_insert);
            assert(udid_insert);
        }

        write_meta_files();
    }

    Task<void> AssetRegistry::scan_async(const std::string path)
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
            path_to_meta_.clear();
            uuid_to_meta_.clear();
        }

        // TODO: TaskGroup<void> group; co_await group; // Wait for all the tasks inside the group
        std::vector<Task<void>> tasks;
        for (const auto& path : vfs_.list(path, ListOptions::FilesOnly | ListOptions::Recursive))
            tasks.push_back(metagen(path));

        for (auto& task : tasks)
            co_await task;

        // Delete all orphaned metas
        {
            std::scoped_lock guard{ mutex_ };

            for (auto it = orphan_metas_.begin(); it != orphan_metas_.end(); ++it)
                vfs_.rm(it->value().path.path());

            orphan_metas_.clear();
        }

        write_meta_files();
    }

    AssetMetadata* AssetRegistry::asset_metadata(const UUID uuid) noexcept
    {
        std::scoped_lock guard{ mutex_ };

        // TODO: Clone or return a pointer?
        if (auto it = uuid_to_meta_.find(uuid); it != uuid_to_meta_.end())
            return it->second;
        return nullptr;
    }

    AssetMetadata* AssetRegistry::asset_metadata(const std::string& path) noexcept
    {
        std::shared_lock guard{ mutex_ };

        // TODO: Clone or return a pointer?
        if (auto it = path_to_meta_.find(util::crypto::fnv1a(path)); it != path_to_meta_.end())
            return it->second;
        return nullptr;
    }

    Task<void> AssetRegistry::metagen(const stdfs::path path) noexcept
    {
        co_await Engine::worker_pool();

        AssetMetadata metadata{};
        metadata.path  = AssetPath{ path.generic_string() };
        metadata.dirty = true;

        const auto ext = path.extension();
        if (ext == ".cxmeta") {
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
                SerializationManager::deserialize_from_json(metadata, jastr);

                if (vfs_.exists(metadata.path.path())) {
                    if (path != path) {
                        // Somehow, the meta file has been moved outside of Codex.
                        // The metafile however is supposed to sit with the asset itself, on the same directory.
                        // Do we move?
                        assert(false);
                    }

                    // We got a meta that is valid, we return it.
                    {
                        std::scoped_lock guard{ mutex_ };

                        auto metabox = Box<AssetMetadata>::make(std::move(metadata));
                        path_to_meta_.try_emplace(util::crypto::fnv1a(metabox->path.path()), metabox.get());
                        uuid_to_meta_.try_emplace(metabox->path.uuid(), metabox.get());
                        log(Debug, "valid meta has been pushed to the registry: {}, {}", metabox->path.path(),
                            metabox->path.uuid());
                        metas_.push_back(std::move(metabox));
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
        } else {
            // It's a possible asset
            auto loader = AssetManager::get_loader_by_ext(ext);
            if (!loader) {
                log(Error, "no loader associated with extension: {}", ext.generic_string());
                co_return;
            }

            trace("possible asset: {}", path.generic_string());

            metadata.type = loader->asset_type_name();

            if (loader->asset_type_id() != typeid(void)) {
                auto handle = vfs_.open(path, { FileMode::Read });
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
            for (auto& e : metas_) {
                assert(e);

                if (e->path.path() == path) {
                    std::scoped_lock guard{ mutex_ };

                    e->checksum = metadata.checksum;
                    e->size     = metadata.size;
                    e->dirty    = true;
                    trace("meta already exists for asset: {}", path.generic_string());
                    co_return;
                }
            }
            trace("maybe orphaned meta for asset: {}", path.generic_string());

            // If we don't have meta then the meta might be orphaned, basically the asset was either renamed or moved.
            for (auto it = orphan_metas_.begin(); it != orphan_metas_.end(); ++it) {
                auto& meta = *it;

                // Asset was moved, update the orphaned meta to point to the new path and pop it from orphaned metas
                // and add it back to the registry.
                if (meta->type == metadata.type && meta->checksum == metadata.checksum && meta->size == metadata.size) {
                    meta->path = AssetPath{ path.generic_string(), meta->path.uuid() };

                    {
                        std::scoped_lock guard{ mutex_ };

                        trace("orphaned asset {} has been resolved", path.generic_string());
                        meta->dirty = true;

                        auto r1 = path_to_meta_.insert_or_assign(util::crypto::fnv1a(meta->path.path()), meta.get());
                        assert(r1.second);

                        auto r2 = uuid_to_meta_.insert_or_assign(meta->path.uuid(), meta.get());
                        assert(r2.second);

                        metas_.push_back(std::move(meta));
                        orphan_metas_.erase(it);
                    }
                    co_return;
                }
            }

            // In this case it has to be a new asset no?
            // Create a new metadata
            trace("new asset registration for: {}", path.generic_string());
            metadata.import_settings = loader->default_import_settings();
            {
                std::scoped_lock guard{ mutex_ };

                auto metabox = Box<AssetMetadata>::make(std::move(metadata));

                auto r1 = path_to_meta_.try_emplace(util::crypto::fnv1a(metabox->path.path()), metabox.get());
                assert(r1.second);

                auto r2 = uuid_to_meta_.try_emplace(metabox->path.uuid(), metabox.get());
                assert(r2.second);

                log(Debug, "valid meta has been pushed to the registry: {}, {}", metabox->path.path(),
                    metabox->path.uuid());
                metas_.push_back(std::move(metabox));
            }
        }
    }

    void AssetRegistry::write_meta_files() const noexcept
    {
        std::shared_lock guard{ mutex_ };

        // TODO: Threaded Parallel range based loop
        for (auto& e : metas_) {
            auto path = stdfs::path{ e->path.path() };
            path.replace_extension(path.extension().generic_string() + ".cxmeta");

            // We don't write metadatas for null assets on the disk.
            if (e->dirty && !e->null_asset) {
                if (auto fh =
                        vfs_.open(path.generic_string(), { FileMode::Create | FileMode::Trunc | FileMode::Write });
                    fh) {
                    auto jastr = SerializationManager::serialize_to_json(*e) + '\n';
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

    void AssetMetadata::serialize(ISerializationNode& node) const
    {
        path.serialize(node);

        node.write("type", type);
        node.write("checksum", checksum);
        node.write("size", size);
        node.write("last_modified", last_modified);

        auto& arr_node = node.begin_array("dependencies");
        for (const auto& e : dependencies)
            e.serialize(node);
        node.end_array();

        if (import_settings)
            import_settings->serialize(node.create_child("import_settings"));
    }

    void AssetMetadata::deserialize(const ISerializationNode& node)
    {
        path.deserialize(node);

        node.read("type", type);
        node.read("checksum", checksum);
        node.read("size", size);
        node.read("last_modified", last_modified);

        node.for_each_array_element(
            [this](const ISerializationNode& node)
            {
                AssetPath path;
                path.deserialize(node);
                dependencies.push_back(std::move(path));
            });

        if (node.has_key("import_settings")) {
            auto loader = AssetManager::get_loader_by_type(type);
            assert(loader);

            import_settings = loader->default_import_settings();
            import_settings->deserialize(node.child("import_settings"));
        }
    }
} // namespace codex
