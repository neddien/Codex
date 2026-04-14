#include "public/asset_manager.h"

#include <engine/graphics/public/texture2d.h>

#define CX_REGISTER_ASSET(asset, ...) // AssetManager::register_loader<loader>(#asset, { __VA_ARGS__ })

namespace codex {
    void AssetManager::init()
    {
        info("AssetManager: Subsystem initialized");

        CX_REGISTER_ASSET(gfx::Texture2D, "png", "jpg", "jpeg");

        info("AssetManager: Registered {} loaders", loaders_.size());
    }

    void AssetManager::dispose()
    {
        loaders_.clear();

        info("AssetManager: Subsystem diposed");
    }

    cc::Task<void> AssetManager::scan(fs::VirtualFilesystem& vfs, const std::string& path)
    {
        return get().registry_.scan(vfs, path);
    }
} // namespace codex
