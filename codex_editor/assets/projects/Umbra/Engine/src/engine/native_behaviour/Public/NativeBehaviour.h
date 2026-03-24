#pragma once

#include <engine/core/public/serializer.h>
#include <engine/reflection/public/reflection.h>
#include <engine/scene/public/entity.inl>

namespace codex {
    // Forward declarations.
    class Entity;
    struct NativeBehaviourComponent;

    class CODEX_API NativeBehaviour : public ISerializable
    {
        friend class Scene;
        friend struct NativeBehaviourComponent;

    public:
        inline void set_owner(const Entity entity) noexcept { parent_ = entity; }

    public:
        virtual ~NativeBehaviour() { lgx::Get("engine").Log(lgx::Info, "~NativeBehaviour()"); };

    public:
        [[nodiscard]] Entity primary_camera_entity() noexcept { return parent_.scene_->primary_camera_entity(); }
        [[nodiscard]] TransformComponent& transform() noexcept;
        [[nodiscard]] auto                create_entity(const std::string_view tag = "default tag")
        {
            return parent_.scene_->create_entity(tag);
        }
        void               remove_entity(Entity entity) { parent_.scene_->remove_entity(entity); }
        [[nodiscard]] auto get_all_entities() { return parent_.scene_->get_all_entities(); }
        [[nodiscard]] auto get_all_entities_with_tag(const std::string_view tag)
        {
            return parent_.scene_->get_all_entities_with_tag(tag);
        }
        [[nodiscard]] auto        entity_count() const noexcept { return parent_.scene_->entity_count(); }
        [[nodiscard]] const auto& current_scene() const noexcept { return parent_.scene_; }
        [[nodiscard]] auto&       current_scene() noexcept { return *parent_.scene_; }

    public:
        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] auto get_all_entities_with_component()
        {
            return parent_.scene_->get_all_entities_with_component<T>();
        }
        template <typename T, typename... TArgs>
        T& add_component(TArgs&&... args)
        {
            return parent_.add_component<T>(std::forward<TArgs>(args)...);
        }
        template <typename T>
        void remove_component()
        {
            parent_.remove_component<T>();
        }
        template <typename T>
        [[nodiscard]] T& get_component()
        {
            return parent_.get_component<T>();
        }
        template <typename T>
        [[nodiscard]] const T& get_component() const
        {
            return parent_.get_component<T>();
        }
        template <typename T>
        [[nodiscard]] bool has_component() const
        {
            return parent_.has_component<T>();
        }

        // FIXME: Mark these methods protected!
        // protected:
    public:
        virtual void                                    on_init() = 0;
        virtual void                                    on_update([[maybe_unused]] const f32 delta_time) {}
        virtual void                                    on_fixed_update([[maybe_unused]] const f32 delta_time) {}
        virtual void                                    on_dispose() {}
        [[nodiscard]] virtual mem::Box<NativeBehaviour> clone() const     = 0;
        [[nodiscard]] virtual const rf::TypeInfo&       type_info() const = 0;

    public:
        ax::EventHandle get_audio_event(const std::string_view event_path);

    public:
        void serialize(ISerializationNode& node) const override;
        void deserialize(const ISerializationNode& node) override;

    protected:
        Entity parent_;
    };
} // namespace codex
