#include "asset_manager.inl"

#include <engine/core/engine.h>
#include <engine/filesystem/vfs.h>
#include <engine/graphics/public/texture2d.h>

namespace codex {
    void AssetManager::init(fs::VirtualFilesystem& vfs, std::string_view asset_root) noexcept
    {
        auto& self       = get();
        self.vfs_        = &vfs;
        self.asset_root_ = asset_root;
        self.registry_   = Box<AssetRegistry>::make(*self.vfs_);

        register_loader<gfx::Texture2DLoader>({ "png", "jpg", "jpeg", "bmp", "gif" });

        self.log(Info, "Initialized with root '{}', {} loaders registered", self.asset_root_, self.loaders_.size());
    }

    void AssetManager::dispose() noexcept
    {
        auto& self = get();

        std::scoped_lock guard{ self.mutex_ };
        self.registry_.reset();
        self.loaders_.clear();
        self.loaders_by_ext_.clear();
        self.loaders_by_type_.clear();
        self.vfs_        = nullptr;
        self.asset_root_ = {};

        self.log(Info, "Disposed");
    }

    Shared<IAssetLoader> AssetManager::get_loader_by_ext(const std::string& extension) noexcept
    {
        auto&            self = get();
        std::shared_lock guard{ self.mutex_ };

        if (extension[0] == '.') {
            const auto nodotext = extension.substr(1);
            if (auto it = self.loaders_by_ext_.find(std::hash<std::string>{}(nodotext));
                it != self.loaders_by_ext_.end())
                return it->second;
        }

        if (auto it = self.loaders_by_ext_.find(std::hash<std::string>{}(extension)); it != self.loaders_by_ext_.end())
            return it->second;
        return nullptr;
    }

    Shared<IAssetLoader> AssetManager::get_loader_by_type(const std::string& type_name) noexcept
    {
        auto&            self = get();
        std::shared_lock guard{ self.mutex_ };

        if (auto it = self.loaders_by_type_.find(std::hash<std::string>{}(type_name));
            it != self.loaders_by_type_.end())
            return it->second;
        return nullptr;
    }

    AssetRegistry& AssetManager::registry() noexcept
    {
        return get().registry_.value();
    }

    Shared<void> AssetManager::load_impl(const std::type_index type, AssetMetadata& meta,
                                         const IAssetImportSettings* params) noexcept
    {
        std::scoped_lock guard{ mutex_ };

        auto fh = vfs_->open(meta.path.path());
        if (!fh)
            return nullptr;

        // Update the metadata
        if (params) {
            log(Debug, "load_impl: params update");
            meta.import_settings = params->clone();
            meta.last_modified   = util::chron::unix_epoch_now();
            meta.dirty           = true;
        }

        if (auto it = loaders_.find(type); it != loaders_.end())
            return it->second->load_asset(std::move(fh), meta.import_settings.get());
        else
            log(Error, "No loader associated with: {}", meta.path.path());

        return nullptr;
    }

    cc::ThreadedExecutor& AssetManager::worker_pool() noexcept
    {
        return Engine::get_worker_pool();
    }
} // namespace codex
