#include <engine/asset_manager/public/asset_common.h>

#include "public/scene.h"

namespace codex {
    class SceneLoader : public AssetLoaderBase<Scene, Scene::ImportSettings>
    {
        [[nodiscard]] Shared<Scene> load(Shared<fs::FileHandle>       fh,
                                         const Scene::ImportSettings& settings) const noexcept override;
    };
} // namespace codex
