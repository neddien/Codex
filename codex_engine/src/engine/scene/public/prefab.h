#pragma once

#include <engine/core/public/serializer.h>
#include <engine/memory/public/memory.h>
#include <engine/scene/public/components.h>
#include <engine/scene/public/entity.h>

namespace codex::scene {
    class CODEX_API Prefab : public ISerializable
    {
    private:
        Prefab() noexcept = default;

    public:
        Prefab(const Prefab&)                = delete;
        Prefab& operator=(const Prefab&)     = delete;
        Prefab(Prefab&&) noexcept            = default;
        Prefab& operator=(Prefab&&) noexcept = default;

    public:
        [[nodiscard]] Entity instantiate(Scene& scene, UUID uuid = UUID{}) const noexcept;

    public:
        [[nodiscard]] static Prefab from_entity(const Entity entity) noexcept;

    public:
        void serialize(ISerializationNode& node) const override;
        void deserialize(const ISerializationNode& node) override;

    private:
        std::vector<mem::Box<Component>> components_;
    };
} // namespace codex::scene
