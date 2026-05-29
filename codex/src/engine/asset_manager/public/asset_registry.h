#pragma once

#include <engine/asset_manager/public/asset_common.h>
#include <engine/core/public/log.h>
#include <engine/core/public/serializer.h>

namespace codex {
    namespace fs {
        class VirtualFilesystem;
    }

    struct AssetMetadata : public ISerializable
    {
        AssetPath                 path;
        std::string               type;
        u32                       checksum;
        u64                       last_modified;
        usize                     size;
        Box<IAssetImportSettings> import_settings;
        std::vector<AssetPath>    dependencies;
        bool                      null_asset;
        mutable bool              dirty;
        mutable std::string       name;

    public:
        AssetMetadata() noexcept = default;
        AssetMetadata(const AssetMetadata& other) noexcept
            : path{ other.path }
            , type{ other.type }
            , checksum{ other.checksum }
            , last_modified{ other.last_modified }
            , size{ other.size }
            , dependencies{ other.dependencies }
            , dirty{ other.dirty }
            , null_asset{ other.null_asset }
        {
            if (other.import_settings)
                import_settings = other.import_settings->clone();

            name = std::filesystem::path{ path.path() }.filename();
        }
        AssetMetadata(AssetMetadata&& other) noexcept
            : path{ std::move(other.path) }
            , type{ std::move(other.type) }
            , checksum{ other.checksum }
            , last_modified{ other.last_modified }
            , size{ other.size }
            , import_settings{ std::move(other.import_settings) }
            , dependencies{ std::move(other.dependencies) }
            , dirty{ other.dirty }
            , name{ other.name }
            , null_asset{ other.null_asset }
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
            std::swap(type, other.type);
            std::swap(last_modified, other.last_modified);
            std::swap(size, other.size);
            import_settings.swap(other.import_settings);
            std::swap(dependencies, other.dependencies);
            std::swap(dirty, other.dirty);
            std::swap(name, other.name);
            std::swap(null_asset, other.null_asset);
        }

    public:
        void serialize(ISerializationNode& node) const;
        void deserialize(const ISerializationNode& node);
    };

    class AssetRegistry : public Loggable<"AssetRegistry">
    {
        using MetadataCallback = std::function<void(const AssetMetadata&)>;

    public:
        AssetRegistry(fs::VirtualFilesystem& vfs) noexcept;
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
        cc::Task<void> scan_async(const std::string path);
        AssetMetadata* asset_metadata(const UUID uuid) noexcept;
        AssetMetadata* asset_metadata(const std::string& path) noexcept;
        void           for_each(MetadataCallback fn) const
        {
            std::scoped_lock guard{ mutex_ };
            for (const Box<AssetMetadata>& e : metas_)
                fn(*e);
        }

    private:
        cc::Task<void> metagen(const std::filesystem::path path) noexcept;
        void           write_meta_files() const noexcept;
        cc::Task<void> write_manifest_async() noexcept;

    private:
        std::unordered_map<UUID, AssetMetadata*>  uuid_to_meta_;
        std::unordered_map<usize, AssetMetadata*> path_to_meta_;
        std::vector<Box<AssetMetadata>>           metas_;
        std::vector<Box<AssetMetadata>>           orphan_metas_;
        mutable std::shared_mutex                 mutex_;
        fs::VirtualFilesystem&                    vfs_;
    };
} // namespace codex
