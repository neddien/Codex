#pragma once

#include <sdafx.h>

#include <Engine/Scene/Public/Components.h>

#define CX_REGISTER_COMPONENT(type) ComponentFactory::Get().Register<type>(#type);

namespace codex {
    class ComponentFactory
    {
    public:
        using DeserializerFn = std::function<void(const ISerializationNode&, const Entity)>;

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
            m_Factories[std::string{ typeName }] = [](const ISerializationNode& node, Entity entity)
            {
                T component;
                component.Deserialize(node);
                entity.AddOrReplaceComponent<T>(component);
            };
        }

        void DeserializeComponent(const std::string& typeName, const ISerializationNode& node, const Entity entity)
        {
            auto it = m_Factories.find(typeName);
            if (it != m_Factories.end())
            {
                it->second(node, entity);
            }
            else
            {
                // TODO: Log warn or something here
            }
        }

    private:
        std::unordered_map<std::string, DeserializerFn> m_Factories;
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
                       GridRendererComponent, TilemapComponent, TilesetAnimationComponent>;

    ///////////////////////////////////////////////////////////////////////////////
} // namespace codex