#pragma once

#include <engine/asset_manager/public/asset_common.h>
#include <engine/core/public/archive.h>
#include <engine/core/public/log.h>

namespace codex {
    namespace fs {
        class VirtualFilesystem;
    }

    struct AssetMetadata : public ISerializable
    {
        AssetPath                 path;
        std::string               storage_path;
        std::string               type;
        u32                       checksum{ 0 };
        u64                       last_modified{ 0 };
        usize                     size{ 0 };
        Box<IAssetImportSettings> import_settings;
        std::vector<AssetPath>    dependencies;
        bool                      null_asset{ false };
        mutable bool              dirty{ false };

    public:
        AssetMetadata() noexcept = default;
        AssetMetadata(const AssetMetadata& other) noexcept
            : path{ other.path }
            , storage_path{ other.storage_path }
            , type{ other.type }
            , checksum{ other.checksum }
            , last_modified{ other.last_modified }
            , size{ other.size }
            , dependencies{ other.dependencies }
            , null_asset{ other.null_asset }
            , dirty{ other.dirty }
        {
            if (other.import_settings)
                import_settings = other.import_settings->clone();
        }
        AssetMetadata(AssetMetadata&& other) noexcept
            : path{ std::move(other.path) }
            , storage_path(std::move(other.storage_path))
            , type{ std::move(other.type) }
            , checksum{ other.checksum }
            , last_modified{ other.last_modified }
            , size{ other.size }
            , import_settings{ std::move(other.import_settings) }
            , dependencies{ std::move(other.dependencies) }
            , null_asset{ other.null_asset }
            , dirty{ other.dirty }
        {
        }

    public:
        AssetMetadata& operator=(const AssetMetadata& other) noexcept { return AssetMetadata{ other }.swap(*this); }
        AssetMetadata& operator=(AssetMetadata&& other) noexcept
        {
            return AssetMetadata{ std::move(other) }.swap(*this);
        }

    public:
        AssetMetadata& swap(AssetMetadata& other) noexcept
        {
            std::swap(path, other.path);
            std::swap(storage_path, other.storage_path);
            std::swap(type, other.type);
            std::swap(last_modified, other.last_modified);
            std::swap(checksum, other.checksum);
            std::swap(size, other.size);
            import_settings.swap(other.import_settings);
            std::swap(dependencies, other.dependencies);
            std::swap(null_asset, other.null_asset);
            std::swap(dirty, other.dirty);
            return *this;
        }

    public:
        void archive(Archive& archive);
    };

    class AssetRegistry : public Loggable<"AssetRegistry">
    {
        using MetadataCallback = std::function<void(const AssetMetadata&)>;

    public:
        AssetRegistry(fs::VirtualFilesystem& vfs, std::string_view root_path) noexcept;
        ~AssetRegistry() noexcept;

    public:
        [[nodiscard]]
        inline fs::VirtualFilesystem& vfs() noexcept
        {
            return vfs_;
        }
        [[nodiscard]]
        inline const fs::VirtualFilesystem& vfs() const noexcept
        {
            return vfs_;
        };

    public:
        void           move_asset(const AssetPath& path, const std::string& new_path);
        void           repath(const AssetPath& path, const std::string& new_path);
        cc::task<void> scan_async(const std::string path);
        AssetMetadata* asset_metadata(const UUID uuid) noexcept;
        AssetMetadata* asset_metadata(const std::string& path) noexcept;
        void           for_each(MetadataCallback fn) const
        {
            std::scoped_lock guard{ mutex_ };
            for (const Box<AssetMetadata>& e : metas_)
                fn(*e);
        }
        cc::task<void> write_manifest_async(const std::string vfs_path) const noexcept;
        cc::task<void> export_assets_async(const std::string vfs_path) const noexcept;
        void           from_manifest(const std::string& path) noexcept;

    private:
        cc::task<void> resolve_meta(const std::filesystem::path path) noexcept;
        cc::task<void> resolve_asset(const std::filesystem::path path) noexcept;
        void           write_meta_files() const noexcept;
        AssetMetadata* append_meta_nolock(AssetMetadata meta) noexcept;
        AssetMetadata* append_meta(AssetMetadata meta) noexcept;

    private:
        // TODO: Use absl::flat_hash_map
        std::unordered_map<UUID, AssetMetadata*>  uuid_to_meta_;
        std::unordered_map<usize, AssetMetadata*> path_to_meta_;
        std::vector<Box<AssetMetadata>>           metas_;
        // FIXME: This doesn't have to be wrapped in a Box btw
        std::vector<Box<AssetMetadata>> orphan_metas_;
        fs::VirtualFilesystem&          vfs_;
        std::string                     root_path_;
        mutable std::shared_mutex       mutex_;
    };
} // namespace codex
