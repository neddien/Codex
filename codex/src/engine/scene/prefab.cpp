#include "public/prefab.h"

#include <engine/core/public/common_third_party_libs.h>
#include <engine/scene/component_factory.h>
#include <engine/scene/public/components.h>
#include <engine/scene/public/scene.h>

namespace codex::scene {
    Entity Prefab::instantiate(Scene& scene, UUID uuid) const noexcept
    {
        auto   registry  = scene.registry_.lock();
        auto   entity    = registry->create();
        Entity cx_entity = Entity{ entity, &scene };

        auto& id_comp   = registry->emplace<IDComponent>(entity, uuid);
        id_comp.parent_ = cx_entity;

        for (auto& c : components_) {
            ComponentFactory::get().instantiate_component(*c, cx_entity);
        }

        return cx_entity;
    }

    Prefab Prefab::from_entity(const Entity entity) noexcept
    {
        // Damn this shit is terrible lol
        // const Component* comp = &entity.first_component();

        // Prefab pf{};

        // while (comp->next_) {
        //     pf.components_.push_back(comp->clone());
        //     comp = comp->next_;
        // }

        // return pf;
        return {};
    }

    void Prefab::serialize(ISerializationNode& node) const
    {
    }

    void Prefab::deserialize(const ISerializationNode& node)
    {
    }
} // namespace codex::scene
