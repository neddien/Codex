#pragma once

#include <engine/asset_manager/public/asset_common.h>
#include <engine/asset_manager/public/asset_registry.h>
#include <engine/concurrency/public/task.h>
#include <engine/concurrency/public/threaded_executor.h>
#include <engine/core/public/log.h>
#include <engine/core/public/system.h>
#include <engine/core/public/uuid.h>
#include <engine/filesystem/public/file_handle.h>
#include <engine/graphics/public/texture2d.h>
#include <engine/memory/public/memory.h>

namespace codex {
    namespace fs {
        class VirtualFilesystem;
    }

    class AssetManager : public System<AssetManager>, public Loggable<"AssetManager">
    {
    public:
        static void init(fs::VirtualFilesystem& vfs, const std::string_view asset_root) noexcept;
        static void dispose() noexcept;

        template <AssetType TAsset>
        [[nodiscard]] static Asset<TAsset> load(const UUID uuid) noexcept
        {
            auto& self = get();
            if (auto maybe_meta = self.registry_->asset_metadata(uuid); maybe_meta) {
                {
                    std::scoped_lock guard{ self.mutex_ };
                    if (auto it = self.cache_.find(maybe_meta->path); it != self.cache_.end())
                        if (auto cached = it->second.lock().template as<TAsset>(); cached)
                            return Asset<TAsset>{ maybe_meta->path, std::move(cached) };
                }

                auto asset = self.load_impl(typeid(TAsset), *maybe_meta).template as<TAsset>();
                if (asset) {
                    auto             handle = Asset<TAsset>{ maybe_meta->path, asset };
                    std::scoped_lock guard{ self.mutex_ };
                    self.cache_.insert_or_assign(maybe_meta->path, handle.as_shared().template as<IAsset>());
                    return handle;
                }
                self.log(Error, "Failed to load asset: {}", uuid);
            } else
                self.log(Error, "Tried loading an unregistered asset: {}", uuid);

            return std::nullopt;
        }
        template <AssetType TAsset>
        [[nodiscard]] static Asset<TAsset> load(const AssetPath& path) noexcept
        {
            return load<TAsset>(path.uuid());
        }
        template <AssetType TAsset, Serializable TParam>
        [[nodiscard]] static Asset<TAsset> load(const UUID uuid, const TParam& param) noexcept
        {
            auto& self = get();
            if (auto maybe_meta = self.registry_->asset_metadata(uuid); maybe_meta) {
                {
                    std::scoped_lock guard{ self.mutex_ };
                    if (auto it = self.cache_.find(maybe_meta->path); it != self.cache_.end())
                        if (auto cached = it->second.lock().template as<TAsset>(); cached)
                            return Asset<TAsset>{ maybe_meta->path, std::move(cached) };
                }

                auto asset = self.load_impl(typeid(TAsset), *maybe_meta, &param).template as<TAsset>();
                if (asset) {
                    auto             handle = Asset<TAsset>{ maybe_meta->path, asset };
                    std::scoped_lock guard{ self.mutex_ };
                    self.cache_.insert_or_assign(maybe_meta->path, handle.as_shared().template as<IAsset>());
                    return handle;
                }
                self.log(Error, "Failed to load asset: {}", uuid);
            } else
                self.log(Error, "Tried loading an unregistered asset: {}", uuid);

            return std::nullopt;
        }
        template <AssetType TAsset, Serializable TParam>
        [[nodiscard]] static Asset<TAsset> load(const AssetPath& path, const TParam& param) noexcept
        {
            return load<TAsset>(path.uuid(), param);
        }
        template <AssetType TAsset>
        [[nodiscard]] static Asset<TAsset> load(const std::string& path) noexcept
        {
            auto&      self     = get();
            const auto abs_path = self.asset_root_ + "/" + path;
            if (auto maybe_meta = self.registry_->asset_metadata(abs_path); maybe_meta) {
                {
                    std::scoped_lock guard{ self.mutex_ };
                    if (auto it = self.cache_.find(maybe_meta->path); it != self.cache_.end())
                        if (auto cached = it->second.lock().template as<TAsset>(); cached)
                            return Asset<TAsset>{ maybe_meta->path, std::move(cached) };
                }

                auto asset = self.load_impl(typeid(TAsset), *maybe_meta).template as<TAsset>();
                if (asset) {
                    auto             handle = Asset<TAsset>{ maybe_meta->path, asset };
                    std::scoped_lock guard{ self.mutex_ };
                    self.cache_.insert_or_assign(maybe_meta->path, handle.as_shared().template as<IAsset>());
                    return handle;
                }
                self.log(Error, "Failed to load asset: {}", path);
            } else
                self.log(Error, "Tried loading an unregistered asset: {}", path);

            return std::nullopt;
        }
        template <AssetType TAsset, Serializable TParam>
        [[nodiscard]] static Asset<TAsset> load(const std::string& path, const TParam& param) noexcept
        {
            auto&      self     = get();
            const auto abs_path = self.asset_root_ + "/" + path;
            if (auto maybe_meta = self.registry_->asset_metadata(abs_path); maybe_meta) {
                {
                    std::scoped_lock guard{ self.mutex_ };
                    if (auto it = self.cache_.find(maybe_meta->path); it != self.cache_.end())
                        if (auto cached = it->second.lock().template as<TAsset>(); cached)
                            return Asset<TAsset>{ maybe_meta->path, std::move(cached) };
                }

                auto asset = self.load_impl(typeid(TAsset), *maybe_meta, &param).template as<TAsset>();
                if (asset) {
                    auto             handle = Asset<TAsset>{ maybe_meta->path, asset };
                    std::scoped_lock guard{ self.mutex_ };
                    self.cache_.insert_or_assign(maybe_meta->path, handle.as_shared().template as<IAsset>());
                    return handle;
                }
                self.log(Error, "Failed to load asset: {}", path);
            } else
                self.log(Error, "Tried loading an unregistered asset: {}", path);

            return std::nullopt;
        }
        template <AssetType TAsset>
        static void unload(const std::string& path) noexcept
        {
            auto&      self     = get();
            const auto abs_path = self.asset_root_ + "/" + path;
            if (auto maybe_meta = self.registry_->asset_metadata(abs_path); maybe_meta) {
                std::scoped_lock guard{ self.mutex_ };
                if (auto it = self.cache_.find(maybe_meta->path); it != self.cache_.end())
                    self.cache_.erase(it);
            } else
                self.log(self.Error, "Tried unloading an alien asset: {}", path);
        }
        template <AssetType TAsset>
        static void unload(const UUID uuid) noexcept
        {
            auto& self = get();
            if (auto maybe_meta = self.registry_->asset_metadata(uuid); maybe_meta) {
                std::scoped_lock guard{ self.mutex_ };
                if (auto it = self.cache_.find(maybe_meta->path); it != self.cache_.end())
                    self.cache_.erase(it);
            } else
                self.log(self.Error, "Tried unloading an alien asset: {}", uuid);
        }
        template <AssetType TAsset, ImportSettingsType TParam>
        static void reimport(const UUID uuid, const TParam& params) noexcept
        {
            auto& self = get();
            if (auto maybe_meta = self.registry_->asset_metadata(uuid); maybe_meta) {
                Ref<IAsset> asset_ref;
                {
                    std::scoped_lock guard{ self.mutex_ };
                    if (auto it = self.cache_.find(maybe_meta->path); it != self.cache_.end())
                        asset_ref = it->second;
                }
                if (auto asset = asset_ref.lock().template as<TAsset>(); asset) {
                    auto asset_new = self.load_impl(typeid(TAsset), *maybe_meta, &params).template as<TAsset>();
                    *asset         = std::move(*asset_new);
                }
                // TODO: Ignore if it isn't cached or isn't valid?
            } else {
                // TODO: Throw exception
            }
        }
        template <AssetType TAsset, ImportSettingsType TParam>
        static void reimport(const AssetPath& path, const TParam& params) noexcept
        {
            reimport<TAsset>(path.uuid(), params);
        }
        template <AssetType TAsset, ImportSettingsType TParam>
        static void reimport(const std::string& path, const TParam& params) noexcept
        {
            auto& self = get();
            if (auto maybe_meta = self.registry_->asset_metadata(path); maybe_meta) {
                Ref<IAsset> asset_ref;
                {
                    std::scoped_lock guard{ self.mutex_ };
                    if (auto it = self.cache_.find(maybe_meta->path); it != self.cache_.end())
                        asset_ref = it->second;
                }
                if (auto asset = asset_ref.lock().template as<TAsset>(); asset) {
                    auto asset_new = self.load_impl(typeid(TAsset), *maybe_meta, &params).template as<TAsset>();
                    *asset         = std::move(*asset_new);
                }
                // TODO: Ignore if it isn't cached or isn't valid?
            } else {
                // TODO: Throw exception
            }
        }

        template <AssetType TAsset>
        [[nodiscard]] static cc::Task<Asset<TAsset>> load_async(UUID uuid) noexcept;
        template <AssetType TAsset>
        [[nodiscard]] static cc::Task<Asset<TAsset>> load_async(AssetPath path) noexcept;
        template <AssetType TAsset>
        [[nodiscard]] static cc::Task<Asset<TAsset>> load_async(std::string path) noexcept;
        template <AssetType TAsset, Serializable TParam>
        [[nodiscard]] static cc::Task<Asset<TAsset>> load_async(UUID uuid, TParam param) noexcept;
        template <AssetType TAsset, Serializable TParam>
        [[nodiscard]] static cc::Task<Asset<TAsset>> load_async(AssetPath path, TParam param) noexcept;
        template <AssetType TAsset, Serializable TParam>
        [[nodiscard]] static cc::Task<Asset<TAsset>> load_async(std::string path, TParam param) noexcept;
        template <AssetType TAsset, ImportSettingsType TParam>
        [[nodiscard]] static cc::Task<void> reimport_async(UUID uuid, TParam params) noexcept;
        template <AssetType TAsset, ImportSettingsType TParam>
        [[nodiscard]] static cc::Task<void> reimport_async(AssetPath path, TParam params) noexcept;
        template <AssetType TAsset, ImportSettingsType TParam>
        [[nodiscard]] static cc::Task<void> reimport_async(std::string path, TParam params) noexcept;

        template <AssetLoader TLoader>
        static void register_loader(std::initializer_list<std::string_view> extensions) noexcept;

        [[nodiscard]] static Shared<IAssetLoader> loader_by_ext(const std::string& extension) noexcept;
        [[nodiscard]] static Shared<IAssetLoader> loader_by_type(const std::string& type_name) noexcept;
        [[nodiscard]] static AssetRegistry&       registry() noexcept;
        [[nodiscard]] static bool                 valid() noexcept { return get().registry_; }
        [[nodiscard]] static std::string          root_dir() noexcept { return get().asset_root_; };

    private:
        [[nodiscard]] Shared<void>                 load_impl(const std::type_index type, AssetMetadata& meta,
                                                             const IAssetImportSettings* params = nullptr) noexcept;
        [[nodiscard]] static cc::ThreadedExecutor& worker_pool() noexcept;

    private:
        fs::VirtualFilesystem*                          vfs_ = nullptr;
        std::string                                     asset_root_;
        std::unordered_map<usize, Shared<IAssetLoader>> loaders_;
        std::unordered_map<usize, Shared<IAssetLoader>> loaders_by_ext_;
        std::unordered_map<usize, Shared<IAssetLoader>> loaders_by_type_;
        std::unordered_map<AssetPath, Ref<IAsset>>      cache_;
        Box<AssetRegistry>                              registry_ = nullptr;
        mutable std::shared_mutex                       mutex_;
    };
} // namespace codex
