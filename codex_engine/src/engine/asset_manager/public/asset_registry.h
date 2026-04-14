#pragma once

#include <engine/asset_manager/public/asset_common.h>
#include <engine/core/public/serializer.h>

namespace codex {
    namespace fs {
        class VirtualFilesystem;
    }

    // template <Serializable TAsset>
    struct AssetMetadata : public ISerializable
    {
        AssetPath              path;
        std::string            type;
        u32                    checksum;
        std::vector<AssetPath> dependencies;
        ISerializable*         import_settings;

    public:
        void serialize(ISerializationNode& node) const;
        void deserialize(const ISerializationNode& node);
    };

    class AssetRegistry
    {
    public:
        void           move_asset(const std::string& old_path, const std::string& new_path);
        cc::Task<void> scan(fs::VirtualFilesystem& vfs, const std::string& path);

    private:
        std::unordered_map<UUID, std::string> uuid_to_path_;
        std::unordered_map<std::string, UUID> path_to_uuid_;
        std::vector<AssetMetadata>            metas_;
    };
} // namespace codex
