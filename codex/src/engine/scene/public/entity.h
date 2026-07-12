#pragma once

#include <engine/concurrency/public/mutex.h>
#include <engine/core/public/common_third_party_libs.h>
#include <engine/core/public/uuid.h>

namespace codex {
    // Forward declarations.
    namespace scene {
        class Prefab;
    } // namespace scene
    class Scene;
    struct Component;
    struct TransformComponent;
    struct IDComponent;

    class CODEX_API Entity
    {
        friend class Scene;
        friend struct SpriteRendererComponent;
        friend class NativeBehaviour;
        friend class scene::Prefab;

    public:
        using handle_type        = u32;
        using signed_handle_type = std::make_signed_t<handle_type>;

    public:
        [[nodiscard]] static constexpr Entity none() noexcept { return Entity{}; }

    public:
        constexpr Entity() = default;
        Entity(const handle_type entity, Scene* const scene)
            : handle_(static_cast<entt::entity>(entity))
            , scene_(scene)
        {
        }
        Entity(const entt::entity entity, Scene* const scene)
            : handle_(entity)
            , scene_(scene)
        {
        }

    public:
        [[nodiscard]] explicit constexpr operator handle_type() const noexcept
        { return static_cast<handle_type>(handle_); }
        [[nodiscard]] inline      operator bool() const noexcept;
        [[nodiscard]] inline bool operator==(const Entity& other) const noexcept { return other.handle_ == handle_; }

    public:
        [[nodiscard]] UUID                      uuid() const noexcept;
        [[nodiscard]] TransformComponent&       transform() noexcept;
        [[nodiscard]] const TransformComponent& transform() const noexcept;
        [[nodiscard]] const Scene*              scene() const noexcept;
        [[nodiscard]] Scene*                    scene() noexcept;

    public:
        template <typename T, typename... TArgs>
            requires(std::is_base_of_v<Component, T>)
        T& add_component(TArgs&&... args);

        template <typename T, typename... TArgs>
            requires(std::is_base_of_v<Component, T>)
        T& add_or_replace_component(TArgs&&... args);

        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        void remove_component();

        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] T& get_component();

        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] const T& get_component() const;

        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] bool has_component() const;

    private:
        entt::entity handle_{ entt::null };
        Scene*       scene_ = nullptr;
    };
} // namespace codex

namespace std {
    template <>
    struct hash<codex::Entity>
    {
        [[nodiscard]] std::size_t operator()(const codex::Entity& entity) const noexcept
        { return std::hash<codex::UUID>{}(entity.uuid()); }
    };
} // namespace std
