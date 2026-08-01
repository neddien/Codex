#pragma once

#include <engine/audio/public/audio.h>
#include <engine/core/public/archive.h>
#include <engine/core/public/log.h>
#include <engine/reflection/public/reflection.h>
#include <engine/scene/public/prefab.h>
#include <engine/scene/public/scene.h>

namespace codex {
    // Forward declarations.
    class Entity;
    struct NativeBehaviourComponent;
    class NBMan;

    class CODEX_API NativeBehaviour : public ISerializable
    {
        friend class Scene;
        friend struct NativeBehaviourComponent;
        friend class NBMan;

    public:
        virtual ~NativeBehaviour() {}

    public:
        [[nodiscard]] Entity primary_camera_entity() noexcept { return parent_.scene_->primary_camera_entity(); }
        [[nodiscard]] TransformComponent& transform() noexcept;
        [[nodiscard]] auto                entities() { return parent_.scene_->entities(); }
        [[nodiscard]] auto                entity_count() const noexcept { return parent_.scene_->entity_count(); }
        [[nodiscard]] const auto&         current_scene() const noexcept { return parent_.scene_; }
        [[nodiscard]] auto&               current_scene() noexcept { return *parent_.scene_; }

    public:
        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] auto entities_with_component()
        { return parent_.scene_->entities_with_component<T>(); }
        template <typename T, typename... TArgs>
        T& add_component(TArgs&&... args)
        { return parent_.add_component<T>(std::forward<TArgs>(args)...); }
        template <typename T>
        void remove_component()
        { parent_.remove_component<T>(); }
        template <typename T>
        [[nodiscard]] T& get_component()
        { return parent_.get_component<T>(); }
        template <typename T>
        [[nodiscard]] const T& get_component() const
        { return parent_.get_component<T>(); }
        template <typename T>
        [[nodiscard]] T* try_get_component()
        { return parent_.try_get_component<T>(); }
        template <typename T>
        [[nodiscard]] const T* try_get_component() const
        { return parent_.try_get_component<T>(); }
        template <typename T>
        [[nodiscard]] bool has_component() const
        { return parent_.has_component<T>(); }

        // FIXME: Mark these methods protected!
        // protected:
    public:
        virtual void                               on_pre_init() {};
        virtual void                               on_init() = 0;
        virtual void                               on_update([[maybe_unused]] const f32 dt) {}
        virtual void                               on_fixed_update([[maybe_unused]] const f32 dt) {}
        virtual void                               on_dispose() {}
        [[nodiscard]] virtual Box<NativeBehaviour> clone() const     = 0;
        [[nodiscard]] virtual const rf::TypeInfo&  type_info() const = 0;

    public:
        [[nodiscard]] Entity              create_entity(const opt<math::transform>& transform = std::nullopt,
                                                        std::string_view tag = "default_tag", UUID uuid = UUID{});
        [[nodiscard]] Entity              create_prefab(const scene::Prefab&        prefab,
                                                        const opt<math::transform>& transform = std::nullopt,
                                                        std::string_view tag = "default tag", UUID uuid = UUID{});
        [[nodiscard]] Entity              create_prefab(const Asset<scene::Prefab>& prefab,
                                                        const opt<math::transform>& transform = std::nullopt,
                                                        std::string_view tag = "default tag", UUID uuid = UUID{});
        void                              remove_entity(Entity entity);
        [[nodiscard]] std::vector<Entity> entities_with_tag(const std::string_view tag);
        [[nodiscard]] ax::EventHandle     audio_event(const std::string_view event_path);
        void                              dispose_self();

    public:
        void archive(Archive& archive) override;

    private:
        inline void set_owner(const Entity entity) noexcept { parent_ = entity; }

    protected:
        Entity                            parent_;
        std::unordered_map<Entity*, UUID> pending_entity_refs_;
    };
} // namespace codex
