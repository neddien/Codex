#pragma once

#include <Engine/Concurrency/Public/Mutex.h>
#include <Engine/Core/Public/Serializer.h>
#include <Engine/Memory/Public/Memory.h>
#include <Engine/Scene/Public/Entity.h>

#include <entt.hpp>

// Forward declarations
class b2World;

namespace codex {
    // Forward declarations
    class Window;
    class Entity;
    class NativeBehaviour;
    namespace scene {
        class EditorCamera;
    } // namespace scene

    class CODEX_API Scene : public ISerializable
    {
        friend class Window;
        friend class Entity;
        friend class Serializer;

    public:
        struct PhysicsProperties
        {
            i32      tickRate           = 60;
            u32      velocityIterations = 6;
            u32      positionIterations = 2;
            f32      scalingFactor      = 1.0f / 64.0f;
            Vector2f gravity            = { 0.0f, -9.8f };
        };
        enum class State
        {
            Play,
            Edit,
            Simulate
        };

    private:
        cc::Mutex<entt::registry> m_Registry;
        std::string               m_Name         = "Default scene";
        mem::Box<b2World>         m_PhysicsWorld = nullptr;
        std::atomic<State>        m_State        = State::Edit;
        std::thread               m_FixedUpdateThread;
        PhysicsProperties         m_PhysicsProperties;
        mem::Box<Entity>          m_PrimaryCameraEntity = nullptr;

    public:
        Scene() noexcept                        = default;
        Scene(const Scene&) noexcept            = delete;
        Scene& operator=(const Scene&) noexcept = delete;
        Scene(Scene&& other) noexcept;
        Scene& operator=(Scene&& other) noexcept;
        ~Scene() noexcept;

    public:
        [[nodiscard]] inline State GetState() const noexcept { return m_State.load(); }
        inline void                SetState(const State newState) noexcept { return m_State.store(newState); }
        [[nodiscard]] inline PhysicsProperties&       GetPhysicsProperties() noexcept { return m_PhysicsProperties; }
        [[nodiscard]] inline const PhysicsProperties& GetPhysicsProperties() const noexcept
        {
            return const_cast<Scene*>(this)->GetPhysicsProperties();
        }
        // TODO: Have a IDisplay trait which allows for
        // displaying the names of the objects just like in UE.
        [[nodiscard]] inline std::string_view GetName() const noexcept { return m_Name; }

    public:
        [[nodiscard]] u32 GetEntityCount() const noexcept;
        void              Swap(Scene& other) noexcept;
        bool              IsValid(const Entity entity) const noexcept;

    public:
        template <typename T>
        std::vector<Entity> GetAllEntitiesWithComponent() noexcept
        {
            auto                view = m_Registry->view<T>();
            std::vector<Entity> entities;
            entities.reserve(view.size());
            for (auto& e : view)
                entities.emplace_back(e, this);
            return entities;
        }

    public:
        void   CopyTo(Scene& other) const noexcept;
        Entity CreateEntity(const std::string_view tag = "default tag", UUID uuid = UUID{}) noexcept;
        void   RemoveEntity(const Entity entity);
        void   RemoveEntity(const u32 entity);
        [[nodiscard]] std::vector<Entity> GetAllEntitesWithTag(const std::string_view tag);
        [[nodiscard]] std::vector<Entity> GetAllEntities();
        [[nodiscard]] Entity              GetPrimaryCameraEntity() noexcept;

    private:
        void RenderSprites();
        void RenderAudio();
        void ConstructPhysicsBodies();

    public:
        void OnEditorInit(scene::EditorCamera& camera);
        void OnRuntimeStart();
        void OnRuntimeStop();
        void OnSimulationStart();
        void OnSimulationStop();
        void OnEditorUpdate(const f32 deltaTime, scene::EditorCamera& camera);
        void OnRuntimeUpdate(const f32 deltaTime);
        void OnSimulationUpdate(const f32 deltaTime, scene::EditorCamera& camera);

    private:
        static void OnFixedUpdate(Scene& self) noexcept;

    public:
        void Serialize(ISerializationNode& node) const override;
        void Deserialize(const ISerializationNode& node) override;
    };
} // namespace codex
