#pragma once

#include <engine/asset_manager/public/asset_common.h>
#include <engine/asset_manager/public/asset_registry.h>
#include <engine/core/public/system.h>
#include <engine/core/public/uuid.h>
#include <engine/filesystem/public/file_handle.h>
#include <engine/memory/public/memory.h>

namespace codex {
    namespace fs {
        class VirtualFilesytem;
    }

    template <typename T>
    class System;

    class AssetManager : public System<AssetManager>
    {
    public:
        template <RegisteredAsset TAsset>
        [[nodiscard]] static Asset<TAsset> load(Shared<fs::FileHandle> fh) noexcept
        {
            Asset<TAsset> asset;
            auto&         inst = get();
            if (auto it = inst.loaders_.find(typeid(TAsset)); it != inst.loaders_.end()) {
                asset = Asset{ fh->path(), it->second->load(std::move(fh)).template as<TAsset>() };
            }

            // We won't reach here.
            return asset;
        }

        template <RegisteredAsset TAsset, typename TParam>
        [[nodiscard]] static Asset<TAsset> load(Shared<fs::FileHandle> fh, const TParam& param) noexcept
        {
            auto& inst = get();
            if (auto it = inst.loaders_.find(typeid(TAsset)); it != inst.loaders_.end()) {
                return Asset{ fh->path(), it->second->load(std::move(fh), &param).template as<TAsset>() };
            }

            // ????

            // We won't reach here.
            return nullptr;
        }

        template <AssetLoader TLoader>
        static void register_loader() noexcept
        {
            get().loaders_.try_emplace(typeid(typename TLoader::asset_type), Box<TLoader>::make());
        }

        static cc::Task<void> scan(fs::VirtualFilesystem& vfs, const std::string& path);

    public:
        void init();
        void dispose();

    private:
        std::unordered_map<std::type_index, Box<IAssetLoader>> loaders_;
        std::unordered_map<UUID, Shared<void>>                 cache_;
        AssetRegistry                                          registry_;
        Shared<fs::VirtualFilesystem>                          vfs_;
    };
} // namespace codex
