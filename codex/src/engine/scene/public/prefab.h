#pragma once

#include <engine/asset_manager/public/asset_manager.h>
#include <engine/core/public/archive.h>
#include <engine/memory/public/memory.h>
#include <engine/reflection/public/reflection.h>
#include <engine/scene/public/entity.h>

namespace codex {
    namespace scene {
        // Forward declarations
        class PrefabLoader;

        class CODEX_API Prefab : public IAsset, public ISerializable
        {
            CX_ASSET(Prefab)

            friend class PrefabLoader;

        public:
            Prefab() noexcept = default;
            Prefab(Entity entity) noexcept;
            Prefab(const Prefab&) noexcept = default;
            Prefab(Prefab&&) noexcept      = default;

        public:
            [[nodiscard]] Entity instantiate(Scene& scene, UUID uuid = UUID{}) const noexcept;

        public:
            [[nodiscard]] static Prefab from_entity(Entity entity) noexcept;

        public:
            void archive(Archive& archive) override;

        private:
            std::vector<u8> serialized_entity_buf_;
        };

        class PrefabLoader final : public AssetLoaderBase<Prefab, void>
        {
            [[nodiscard]] Shared<Prefab> load(Shared<fs::FileHandle> fh) const noexcept override;
        };
    } // namespace scene

    RF_REGISTER_TYPE(Asset<scene::Prefab>, codex::rf::PropertyType::PrefabAsset)

    // Archives an Asset<Prefab> as its AssetPath and resolves it through the
    // AssetManager on load, so scripts can hold prefab references as plain
    // RF_PROPERTY members (assigned via drag and drop in the editor).
    // An unassigned handle is stored with a null uuid (UUID{} generates a random
    // one, which would break save determinism and read as a valid reference).
    inline void serialize(Archive& ar, Asset<scene::Prefab>& prefab)
    {
        AssetPath path = ar.saving() && prefab.valid() ? prefab.path() : AssetPath{ "", UUID{ 0 } };
        ar("path", path);
        if (ar.loading())
            prefab = path ? AssetManager::load<scene::Prefab>(path.uuid()) : Asset<scene::Prefab>{};
    }
} // namespace codex
