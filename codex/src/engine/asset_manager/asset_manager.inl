#pragma once

#include "public/asset_manager.h"

namespace codex {
    template <AssetLoader TLoader>
    void AssetManager::register_loader(std::initializer_list<std::string_view> extensions) noexcept
    {
        auto&            self = get();
        std::scoped_lock guard{ self.mutex_ };

        auto loader = Shared<TLoader>::make();

        self.loaders_.try_emplace(loader->asset_type_id(), loader.template as<TLoader>());
        for (const auto& e : extensions) {
            bool result = self.loaders_by_ext_.try_emplace(std::hash<std::string_view>{}(e), loader).second;
            assert(result);
        }

        bool result =
            self.loaders_by_type_.try_emplace(std::hash<std::string_view>{}(loader->asset_type_name()), loader).second;
        assert(result);
    }

    template <AssetType TAsset>
    cc::Task<Asset<TAsset>> AssetManager::load_async(UUID uuid) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(uuid);
    }
    template <AssetType TAsset>
    cc::Task<Asset<TAsset>> AssetManager::load_async(AssetPath path) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(path);
    }
    template <AssetType TAsset>
    cc::Task<Asset<TAsset>> AssetManager::load_async(std::string path) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(path);
    }
    template <AssetType TAsset, Serializable TParam>
    cc::Task<Asset<TAsset>> AssetManager::load_async(UUID uuid, TParam param) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(uuid, param);
    }
    template <AssetType TAsset, Serializable TParam>
    cc::Task<Asset<TAsset>> AssetManager::load_async(AssetPath path, TParam param) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(path, param);
    }
    template <AssetType TAsset, Serializable TParam>
    cc::Task<Asset<TAsset>> AssetManager::load_async(std::string path, TParam param) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(path, param);
    }
    template <AssetType TAsset, ImportSettingsType TParam>
    cc::Task<void> AssetManager::reimport_async(UUID uuid, TParam params) noexcept
    {
        co_await worker_pool();
        reimport<TAsset>(uuid, params);
    }
    template <AssetType TAsset, ImportSettingsType TParam>
    cc::Task<void> AssetManager::reimport_async(AssetPath path, TParam params) noexcept
    {
        co_await worker_pool();
        reimport<TAsset>(path, params);
    }
    template <AssetType TAsset, ImportSettingsType TParam>
    cc::Task<void> AssetManager::reimport_async(std::string path, TParam params) noexcept
    {
        co_await worker_pool();
        reimport<TAsset>(path, params);
    }
} // namespace codex
