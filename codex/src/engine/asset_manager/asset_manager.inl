#pragma once

#include "public/asset_manager.h"

namespace codex {
    template <AssetLoader TLoader>
    void AssetManager::register_loader(std::initializer_list<std::string_view> extensions) noexcept
    {
        auto&            self = get();
        std::scoped_lock guard{ self.mutex_ };

        auto loader = Shared<TLoader>::make();

        self.loaders_.try_emplace(loader->asset_type_hash(), loader.template as<TLoader>());
        for (const auto& e : extensions) {
            bool result = self.loaders_by_ext_.try_emplace(util::crypto::fnv1a(e), loader).second;
            assert(result);
        }

        bool result = self.loaders_by_type_.try_emplace(util::crypto::fnv1a(loader->asset_type_name()), loader).second;
        assert(result);
    }

    template <AssetType TAsset>
    cc::task<Asset<TAsset>> AssetManager::load_async(UUID uuid) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(uuid);
    }
    template <AssetType TAsset>
    cc::task<Asset<TAsset>> AssetManager::load_async(AssetPath path) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(path);
    }
    template <AssetType TAsset>
    cc::task<Asset<TAsset>> AssetManager::load_async(std::string path) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(path);
    }
    template <AssetType TAsset, Serializable TParam>
    cc::task<Asset<TAsset>> AssetManager::load_async(UUID uuid, TParam param) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(uuid, param);
    }
    template <AssetType TAsset, Serializable TParam>
    cc::task<Asset<TAsset>> AssetManager::load_async(AssetPath path, TParam param) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(path, param);
    }
    template <AssetType TAsset, Serializable TParam>
    cc::task<Asset<TAsset>> AssetManager::load_async(std::string path, TParam param) noexcept
    {
        co_await worker_pool();
        co_return load<TAsset>(path, param);
    }
    template <AssetType TAsset, ImportSettingsType TParam>
    cc::task<void> AssetManager::reimport_async(UUID uuid, TParam params) noexcept
    {
        co_await worker_pool();
        reimport<TAsset>(uuid, params);
    }
    template <AssetType TAsset, ImportSettingsType TParam>
    cc::task<void> AssetManager::reimport_async(AssetPath path, TParam params) noexcept
    {
        co_await worker_pool();
        reimport<TAsset>(path, params);
    }
    template <AssetType TAsset, ImportSettingsType TParam>
    cc::task<void> AssetManager::reimport_async(std::string path, TParam params) noexcept
    {
        co_await worker_pool();
        reimport<TAsset>(path, params);
    }
} // namespace codex
