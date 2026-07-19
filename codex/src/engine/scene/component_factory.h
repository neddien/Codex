#pragma once

#include <engine/core/public/archive.h>
#include <engine/core/public/exception.h>
#include <engine/native_behaviour/public/native_behaviour.h>
#include <engine/scene/public/components.h>
#include <engine/scene/public/entity.h>

#define CX_REGISTER_COMPONENT(type) ComponentFactory::get().register_component<type>(#type);

namespace codex {
    class ComponentFactory : public System<ComponentFactory>
    {
    public:
        using DeserializerFn = std::function<void(Archive&, Entity)>;
        using InstantiateFn  = std::function<void(const Component&, Entity)>;

    public:
        template <typename T>
        void register_component(const std::string_view type_name) noexcept
        {
            deser_factories_[std::string{ type_name }] = [](Archive& ar, Entity entity)
            {
                T component;
                component.parent_ = entity;
                component.archive(ar);
                entity.add_or_replace_component<T>(std::move(component));
            };

            // inst_factories_[std::string{ type_name }] = [](const Component& component, Entity entity)
            //{ entity.add_or_replace_component<T>(component); };
        }

        void deserialize_component(const std::string& type_name, Archive& ar, const Entity entity)
        {
            auto it = deser_factories_.find(type_name);
            if (it != deser_factories_.end()) {
                it->second(ar, entity);
            } else {
                throw DeserializationException(
                    "Component {} has not been registered therefore could not be deserialized.", type_name);
            }
        }

        void instantiate_component(const Component& component, Entity entity)
        {
            auto it = inst_factories_.find(std::string{ component.type_name() });
            if (it != inst_factories_.end()) {
                it->second(component, entity);
            } else {
                throw DeserializationException(
                    "Component {} has not been registered therefore could not be deserialized.", component.type_name());
            }
        }

    private:
        std::unordered_map<std::string, DeserializerFn> deser_factories_;
        std::unordered_map<std::string, InstantiateFn>  inst_factories_;
    };

    /////////////// NOTE: Update these when adding new components! //////////////

    inline void register_all_components() noexcept
    {
        CX_REGISTER_COMPONENT(IDComponent);
        CX_REGISTER_COMPONENT(TagComponent);
        CX_REGISTER_COMPONENT(TransformComponent);
        CX_REGISTER_COMPONENT(SpriteRendererComponent);
        CX_REGISTER_COMPONENT(NativeBehaviourComponent);
        CX_REGISTER_COMPONENT(CameraComponent);
        CX_REGISTER_COMPONENT(RigidBody2DComponent);
        CX_REGISTER_COMPONENT(BoxCollider2DComponent);
        CX_REGISTER_COMPONENT(CircleCollider2DComponent);
        CX_REGISTER_COMPONENT(RevoluteJoint2DComponent);
        CX_REGISTER_COMPONENT(GridRendererComponent);
        CX_REGISTER_COMPONENT(TilemapComponent);
        CX_REGISTER_COMPONENT(TilesetAnimationComponent);
        CX_REGISTER_COMPONENT(AudioSourceComponent);
        CX_REGISTER_COMPONENT(AudioListenerComponent);
        CX_REGISTER_COMPONENT(HierarchyComponent);
    }

    template <typename... Components>
    struct ComponentGroup
    {
    };

    // NOTE: Do not forget to add a new entry for Scene::entities_with_component<T>() and AllComponents<T...>
    // when adding a new component!
    using AllComponents =
        ComponentGroup<IDComponent, TransformComponent, TagComponent, SpriteRendererComponent, NativeBehaviourComponent,
                       CameraComponent, RigidBody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent,
                       GridRendererComponent, TilemapComponent, TilesetAnimationComponent, AudioSourceComponent,
                       AudioListenerComponent, HierarchyComponent, RevoluteJoint2DComponent>;

    ///////////////////////////////////////////////////////////////////////////////
} // namespace codex
