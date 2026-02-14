#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/Exception.h>
#include <Engine/Core/Public/Serializer.h>
#include <Engine/Scene/Public/Components.h>
#include <Engine/Scene/Public/Entity.h>

#define CX_REGISTER_COMPONENT(type) ComponentFactory::Get().Register<type>(#type);

namespace codex {
    class ComponentFactory
    {
    public:
        using DeserializerFn = std::function<void(const ISerializationNode&, Entity)>;
        using InstantiateFn  = std::function<void(const Component&, Entity)>;

    public:
        static ComponentFactory& Get()
        {
            static ComponentFactory instance;
            return instance;
        }

    public:
        template <typename T>
        void Register(const std::string_view typeName) noexcept
        {
            m_DeserFactories[std::string{ typeName }] = [](const ISerializationNode& node, Entity entity)
            {
                T component;
                component.Deserialize(node);
                entity.AddOrReplaceComponent<T>(std::move(component));
            };

            // m_InstFactories[std::string{ typeName }] = [](const Component& component, Entity entity)
            //{ entity.AddOrReplaceComponent<T>(component); };
        }

        void DeserializeComponent(const std::string& typeName, const ISerializationNode& node, const Entity entity)
        {
            auto it = m_DeserFactories.find(typeName);
            if (it != m_DeserFactories.end())
            {
                it->second(node, entity);
            }
            else
            {
                cx_throw(DeserializationException,
                         "Component {} has not been registered therefore could not be deserialized.", typeName);
            }
        }

        void InstantiateComponent(const Component& component, Entity entity)
        {
            auto it = m_InstFactories.find(std::string{ component.TypeName() });
            if (it != m_InstFactories.end())
            {
                it->second(component, entity);
            }
            else
            {
                cx_throw(DeserializationException,
                         "Component {} has not been registered therefore could not be deserialized.",
                         component.TypeName());
            }
        }

    private:
        std::unordered_map<std::string, DeserializerFn> m_DeserFactories;
        std::unordered_map<std::string, InstantiateFn>  m_InstFactories;
    };

    /////////////// NOTE: Updates these when adding new components! //////////////

    inline void RegisterAllComponents() noexcept
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
        CX_REGISTER_COMPONENT(GridRendererComponent);
        CX_REGISTER_COMPONENT(TilemapComponent);
        CX_REGISTER_COMPONENT(TilesetAnimationComponent);
        CX_REGISTER_COMPONENT(AudioSourceComponent);
        CX_REGISTER_COMPONENT(AudioListenerComponent);
    }

    template <typename... Components>
    struct ComponentGroup
    {
    };

    // NOTE: Do not forget to add a new entry for Scene::GetAllEntitiesWithComponent<T>() and for AllComponents<T...>
    // when adding a new component!
    using AllComponents =
        ComponentGroup<IDComponent, TransformComponent, TagComponent, SpriteRendererComponent, NativeBehaviourComponent,
                       CameraComponent, RigidBody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent,
                       GridRendererComponent, TilemapComponent, TilesetAnimationComponent, AudioSourceComponent,
                       AudioListenerComponent>;

    ///////////////////////////////////////////////////////////////////////////////
} // namespace codex
