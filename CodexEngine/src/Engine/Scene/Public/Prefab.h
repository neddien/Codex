#pragma once

#include <Engine/Core/Public/Serializer.h>
#include <Engine/Memory/Public/Memory.h>
#include <Engine/Scene/Public/Components.h>
#include <Engine/Scene/Public/Entity.h>

// Prefab pf { node };
// ISerializationNode node;
// Entity e = e.Serialize(node);
// auto entityv2 = Scene::InstantiatePrefab(prefab);

namespace codex::scene {
    class CODEX_API Prefab : public ISerializable
    {
    private:
        Prefab() noexcept = default;

    public:
        Prefab(const Prefab&)            = delete;
        Prefab& operator=(const Prefab&) = delete;
        Prefab(Prefab&&) noexcept        = default;
        Prefab& operator=(Prefab&&) noexcept = default;

    public:
        Entity Instantiate(Scene& scene, UUID uuid = UUID{}) const noexcept;

    public:
        static Prefab FromEntity(const Entity entity) noexcept;

    public:
        void Serialize(ISerializationNode& node) const;
        void Deserialize(const ISerializationNode& node);

    private:
        std::vector<mem::Box<Component>> m_Components;
    };
} // namespace codex::scene
