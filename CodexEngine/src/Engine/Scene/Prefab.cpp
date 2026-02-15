#include "Public/Prefab.h"

#include <Engine/Scene/ComponentFactory.h>
#include <Engine/Scene/Public/Scene.h>

#include <entt.hpp>

namespace codex::scene {
    Entity Prefab::Instantiate(Scene& scene, UUID uuid) const noexcept
    {
        auto   registry  = scene.m_Registry.Lock();
        auto   entity    = registry->create();
        Entity cx_entity = Entity{ entity, &scene };

        auto& id_comp    = registry->emplace<IDComponent>(entity, uuid);
        id_comp.m_Parent = cx_entity;

        for (auto& c : m_Components)
        {
            ComponentFactory::Get().InstantiateComponent(*c, cx_entity);
        }

        return cx_entity;
    }

    Prefab Prefab::FromEntity(const Entity entity) noexcept
    {
        const Component* comp = &entity.GetFirstComponent();

        Prefab pf{};

        while (comp->m_Next)
        {
            pf.m_Components.push_back(comp->Clone());
            comp = comp->m_Next;
        }

        return pf;
    }

    void Prefab::Serialize(ISerializationNode& node) const
    {
    }

    void Prefab::Deserialize(const ISerializationNode& node)
    {
    }
} // namespace codex::scene
