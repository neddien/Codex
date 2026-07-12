#include "public/prefab.h"

#include <engine/core/public/binary_archive.h>
#include <engine/core/public/common_third_party_libs.h>
#include <engine/scene/component_factory.h>
#include <engine/scene/public/components.h>
#include <engine/scene/public/entity.inl>
#include <engine/scene/public/scene.h>

namespace codex::scene {
    Prefab::Prefab(Entity entity)
    {
        if (entity) {
            BinaryArchiveBackend binsd{}; // Automatically save mode (ie saving_ = false)
            Archive              ar{ binsd };
            serialize(ar, entity);

            serialized_entity_buf_ = binsd.take_buffer();
        }
    }

    Entity Prefab::instantiate(Scene& scene, UUID uuid) const noexcept
    {
        Entity cx_entity = scene.create_entity(std::nullopt, "default tag", uuid);

        BinaryArchiveBackend binsd{ serialized_entity_buf_ }; // Automatically load mode (ie saving_ = true)
        Archive              ar{ binsd };
        serialize(ar, cx_entity);

        cx_entity.add_or_replace_component<IDComponent>().uuid = uuid;

        return cx_entity;
    }

    Prefab Prefab::from_entity(Entity entity) noexcept
    {
        Prefab prefab{};
        if (entity) {
            BinaryArchiveBackend binsd{}; // Automatically save mode (ie saving_ = false)
            Archive              ar{ binsd };
            serialize(ar, entity);

            prefab.serialized_entity_buf_ = binsd.take_buffer();
        }

        return prefab;
    }

    void Prefab::archive(Archive& ar)
    { ar("binser_entity", serialized_entity_buf_); }

    Shared<Prefab> PrefabLoader::load(Shared<fs::FileHandle> fh) const noexcept
    {
        auto prefab = Shared<Prefab>{ new Prefab };

        // We only use binary serializer for prefabs
        // FIXME: In the future we must always read files by chunks!
        std::vector<u8> buf(fh->size());
        fh->read(buf.data(), buf.size());

        BinaryArchiveBackend binsd{ buf };
        Archive              ar{ binsd };
        prefab->archive(ar);

        return prefab;
    }
} // namespace codex::scene
