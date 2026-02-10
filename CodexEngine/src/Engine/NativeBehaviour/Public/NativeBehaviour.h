#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/Serializer.h>
#include <Engine/Reflection/Public/Reflection.h>
#include <Engine/Scene/Public/Entity.inl>

namespace codex {
    // Forward declarations.
    class Entity;
    struct NativeBehaviourComponent;

    class CODEX_API NativeBehaviour : public ISerializable
    {
        friend class Scene;
        friend struct NativeBehaviourComponent;

    protected:
        Entity m_Parent;

    public:
        inline void SetOwner(const Entity entity) noexcept { m_Parent = entity; }

    public:
        virtual ~NativeBehaviour() { lgx::Get("engine").Log(lgx::Info, "~NativeBehaviour()"); };

    public:
        [[nodiscard]] Entity GetPrimaryCameraEntity() noexcept { return m_Parent.m_Scene->GetPrimaryCameraEntity(); }
        [[nodiscard]] TransformComponent& GetTransform() noexcept;
        auto CreateEntity(const std::string_view tag = "default tag") { return m_Parent.m_Scene->CreateEntity(tag); }
        void RemoveEntity(Entity entity) { m_Parent.m_Scene->RemoveEntity(entity); }
        [[nodiscard]] auto GetAllEntities() { return m_Parent.m_Scene->GetAllEntities(); }
        [[nodiscard]] auto GetAllEntitiesWithTag(const std::string_view tag)
        {
            return m_Parent.m_Scene->GetAllEntitesWithTag(tag);
        }
        [[nodiscard]] auto        GetEntityCount() const noexcept { return m_Parent.m_Scene->GetEntityCount(); }
        [[nodiscard]] const auto& GetCurrentScene() const noexcept { return m_Parent.m_Scene; }
        [[nodiscard]] auto&       GetCurrentScene() noexcept { return *m_Parent.m_Scene; }

    public:
        template <typename T>
            requires(std::is_base_of_v<Component, T>)
        [[nodiscard]] auto GetAllEntitiesWithComponent()
        {
            return m_Parent.m_Scene->GetAllEntitiesWithComponent<T>();
        }
        template <typename T, typename... TArgs>
        T& AddComponent(TArgs&&... args)
        {
            return m_Parent.AddComponent<T>(std::forward<TArgs>(args)...);
        }
        template <typename T>
        void RemoveComponent()
        {
            m_Parent.RemoveComponent<T>();
        }
        template <typename T>
        [[nodiscard]] T& GetComponent()
        {
            return m_Parent.GetComponent<T>();
        }
        template <typename T>
        [[nodiscard]] const T& GetComponent() const
        {
            return reinterpret_cast<NativeBehaviour*>(this->GetComponent<T>());
        }
        template <typename T>
        [[nodiscard]] bool HasComponent() const
        {
            return m_Parent.HasComponent<T>();
        }

        // FIXME: Mark these methods protected!
        // protected:
    public:
        virtual void                                    OnInit() = 0;
        virtual void                                    OnUpdate([[maybe_unused]] const f32 deltaTime) {}
        virtual void                                    OnFixedUpdate([[maybe_unused]] const f32 deltaTime) {}
        virtual void                                    OnDispose() {}
        [[nodiscard]] virtual mem::Box<NativeBehaviour> Clone() const       = 0;
        [[nodiscard]] virtual const rf::TypeInfo&       GetTypeInfo() const = 0;

    public:
        ax::EventHandle GetAudioEvent(const std::string_view eventPath);

    public:
        void Serialize(ISerializationNode& node) const override;
        void Deserialize(const ISerializationNode& node) override;
    };
} // namespace codex
