#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/UUID.h>
#include <Engine/Concurrency/Public/Mutex.h>

#include <entt.hpp>

namespace codex {
    // Forward declerations.
    class Scene;
    struct Component;
    struct TransformComponent;
    struct IDComponent;

    class CODEX_API Entity
    {
        friend class Scene;
        friend struct SpriteRendererComponent;
        friend class NativeBehaviour;

    public:
        using HandleType       = u32;
        using SingedHandleType = std::make_signed_t<HandleType>();

    private:
        entt::entity m_Handle{ entt::null };
        Scene*       m_Scene = nullptr;

    public:
        constexpr Entity() = default;
        Entity(const HandleType entity, Scene* const scene)
            : m_Handle(static_cast<entt::entity>(entity))
            , m_Scene(scene)
        {
        }
        Entity(const entt::entity entity, Scene* const scene)
            : m_Handle(entity)
            , m_Scene(scene)
        {
        }

    public:
        [[nodiscard]] static constexpr Entity None() noexcept { return Entity{}; }

    public:
        [[nodiscard]] explicit constexpr operator HandleType() const noexcept
        {
            return static_cast<HandleType>(m_Handle);
        }

    public:
        [[nodiscard]] operator bool() const noexcept;
        [[nodiscard]] bool operator==(const Entity& other) const noexcept { return other.m_Handle == m_Handle; }

    public:
        [[nodiscard]] UUID                      GetUUID() const noexcept;
        [[nodiscard]] TransformComponent&       GetTransform() noexcept;
        [[nodiscard]] const TransformComponent& GetTransform() const noexcept;

    public:
        template <typename T, typename... TArgs>
            requires(std::is_base_of_v<Component, T>)
        T& AddComponent(TArgs&&... args);
        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        void RemoveComponent();
        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] T& GetComponent();
        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] const T& GetComponent() const;
        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] bool HasComponent() const;
    };
} // namespace codex

namespace std {
    template <>
    struct hash<codex::Entity>
    {
        [[nodiscard]] std::size_t operator()(const codex::Entity& entity) const noexcept
        {
            return std::hash<codex::UUID>{}(entity.GetUUID());
        }
    };
} // namespace std
