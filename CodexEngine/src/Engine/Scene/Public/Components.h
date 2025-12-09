#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/UUID.h>
#include <Engine/Memory/Public/Memory.h>
#include <Engine/NativeBehaviour/Public/NativeBehaviour.h>
#include <Engine/Physics/Public/PhysicsMaterial2D.h>

#include "Camera.h"
#include "Sprite.h"

namespace codex {
    // Forward decelerations
    class Scene;
    class Entity;
    class Serializer;

    struct Component
    {
        CX_COMPONENT

    protected:
        virtual void OnInit() {}
        virtual ~Component() = default;
    };

    struct IDComponent : public Component
    {
    public:
        UUID uuid;

    public:
        IDComponent() noexcept = default;
        IDComponent(UUID uuid) noexcept
            : uuid(std::move(uuid))
        {
        }
    };

    struct CODEX_API TagComponent : public Component
    {
        CX_COMPONENT

    public:
        std::string tag;

    public:
        TagComponent();
        TagComponent(const std::string_view tag);
    };

    struct CODEX_API TransformComponent : public Component
    {
        CX_COMPONENT

    public:
        Vector3f position;
        Vector3f rotation;
        Vector3f scale;

    public:
        TransformComponent(const Vector3f position = Vector3f{}, const Vector3f rotation = Vector3f{},
                           const Vector3f scale = Vector3f{ 1.0f, 1.0f, 1.0f });

    public:
        [[nodiscard]] inline Matrix4f ToMatrix() const noexcept
        {
            Matrix4f transform_mat = glm::identity<Matrix4f>();
            transform_mat          = glm::translate(transform_mat, position);
            transform_mat          = glm::rotate(transform_mat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            transform_mat          = glm::rotate(transform_mat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            transform_mat          = glm::rotate(transform_mat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            transform_mat          = glm::scale(transform_mat, scale);
            return transform_mat;
        }
    };

    struct CODEX_API SpriteRendererComponent : public Component
    {
        CX_COMPONENT

    private:
        // TODO: Handle sprite renderers that do not have an actual sprite.
        Sprite m_Sprite;

    public:
        SpriteRendererComponent() = default;
        SpriteRendererComponent(Sprite sprite);

    public:
        inline Sprite&       GetSprite() noexcept { return m_Sprite; }
        inline const Sprite& GetSprite() const noexcept { return m_Sprite; }
    };

    CX_CUSTOM_EXCEPTION(ScriptException, "An unknown behaviour exception occured.")
    CX_CUSTOM_EXCEPTION(DuplicateBehaviourException, "Cnnaot have more than one type of behaviour on a single entity.")

    struct CODEX_API NativeBehaviourComponent : public Component
    {
        CX_COMPONENT

    public:
        using BehaviourMap  = std::unordered_map<std::string, mem::Box<NativeBehaviour>>;
        using BehaviourList = std::vector<NativeBehaviour*>;

    private:
        BehaviourMap  m_Behaviours;
        BehaviourList m_BehaviourList;

    public:
        NativeBehaviourComponent() noexcept = default;
        NativeBehaviourComponent(const NativeBehaviourComponent& other);
        NativeBehaviourComponent& operator=(const NativeBehaviourComponent& other);
        NativeBehaviourComponent(NativeBehaviourComponent&& other) noexcept            = default;
        NativeBehaviourComponent& operator=(NativeBehaviourComponent&& other) noexcept = default;
        ~NativeBehaviourComponent() noexcept { DisposeBehaviours(); }

    public:
        inline void Swap(NativeBehaviourComponent& other) noexcept
        {
            std::swap(m_Behaviours, other.m_Behaviours);
            std::swap(m_BehaviourList, other.m_BehaviourList);
        }
        [[nodiscard]] inline BehaviourMap&       GetBehaviours() noexcept { return m_Behaviours; }
        [[nodiscard]] inline const BehaviourMap& GetBehaviours() const noexcept
        {
            return const_cast<NativeBehaviourComponent*>(this)->GetBehaviours();
        }
        template <typename T>
        [[nodiscard]] inline T* GetBehaviour() noexcept
        {
            for (const auto e : m_BehaviourList)
            {
                if (typeid(T) == typeid(e))
                {
                    return reinterpret_cast<T*>(e);
                }
            }

            return nullptr;
        }
        template <typename T>
        [[nodiscard]] inline const T& GetBehaviour() const noexcept
        {
            for (const auto e : m_BehaviourList)
            {
                if (typeid(T) == typeid(e))
                {
                    return *e;
                }
            }
        }

    public:
        void                      OnInit() override;
        void                      Attach(mem::Box<NativeBehaviour> bh);
        mem::Box<NativeBehaviour> Detach(const std::string& className);
        void                      InstantiateBehaviour(const std::string& className);
        void                      Dispose(const std::string& className);

    public:
        inline void DisposeBehaviours()
        {
            m_Behaviours.clear();
            m_BehaviourList.clear();
        }
        inline void OnUpdate(const f32 deltaTime)
        {
            // TODO: This guy should NOT be inline, it throws an exception?
            for (auto& e : m_BehaviourList)
                e->OnUpdate(deltaTime);
        }
        inline void OnFixedUpdate(const f32 deltaTime)
        {
            // TODO: This guy should NOT be inline, it throws an exception?
            for (auto& e : m_BehaviourList)
                e->OnFixedUpdate(deltaTime);
        }
        inline void SetParent(const Entity entity) const noexcept
        {
            for (auto& e : m_BehaviourList)
                e->m_Parent = entity;
        }

    public:
        template <typename T, typename... TArgs>
        T& New(TArgs&&... args)
            requires(std::is_base_of_v<NativeBehaviour, T>)
        {
            for (const auto& [k, v] : m_Behaviours)
            {
                if (typeid(v) == typeid(T))
                    cx_throwd(DuplicateBehaviourException);
            }

            mem::Box<NativeBehaviour> bh(new T(std::forward<TArgs>(args)...));
            bh->OnInit();
            // FIXME: Serialize properly w new serializaiton system
            //bh->Serialize();
            /*
            const std::string& name = bh->m_SerializedData.begin().key();
            if (!m_Behaviours.contains(name))
                m_Behaviours[name] = std::move(bh);
            */

            return *((T*)bh.Get());
            //return *reinterpret_cast<T*>(m_Behaviours[name].Get());
        }
    };

    struct CameraComponent : public Component
    {
        CX_COMPONENT

    public:
        scene::Camera camera;
        bool          primary = true;
        Vector3f      focalPoint{ 0.0f };
    };

    struct CODEX_API RigidBody2DComponent : public Component
    {
        CX_COMPONENT

    public:
        enum class BodyType
        {
            Static,
            Dynamic,
            Kinematic
        } bodyType           = BodyType::Static;
        bool  fixedRotation  = false;
        f32   linearDamping  = 0.0f;
        f32   angularDamping = 0.1f;
        bool  highVelocity   = false;
        bool  enabled        = true;
        f32   gravityScale   = 1.0f;
        void* runtimeBody    = nullptr;

    public:
        void ApplyForce(const Vector2f& force, const std::optional<Vector2f> point = std::nullopt) noexcept;
        void ApplyTorque(const f32 torque) noexcept;
        void ApplyLinearImpulse(const Vector2f& impulse, const std::optional<Vector2f> point = std::nullopt);
        void ApplyAngularImpulse(const f32 torque);
    };

    struct BoxCollider2DComponent : public Component
    {
        CX_COMPONENT

    public:
        Vector2f offset{ 0.0f, 0.0f };
        Vector2f size{ 32.0f, 32.0f };

        phys::PhysicsMaterial2D physicsMaterial;

        void* runtimeFixture = nullptr;
    };

    struct CircleCollider2DComponent : public Component
    {
        CX_COMPONENT

    public:
        Vector2f offset{ 0.0f, 0.0f };
        f32      radius = 32.0f;

        phys::PhysicsMaterial2D physicsMaterial;

        void* runtimeFixture = nullptr;
    };

    struct CODEX_API GridRendererComponent : public Component
    {
        CX_COMPONENT

    public:
        Vector2f cellSize{ 64.0f, 64.0f };
        Vector4f colour{ 1.0f, 1.0f, 1.0f, 0.3f };
    };

    struct CODEX_API TilemapComponent : public Component
    {
        CX_COMPONENT

    public:
        enum class State
        {
            Brush,
            Erase
        };
        struct Tile
        {
            Vector3f pos{ 0.0f, 0.0f, 0.0f };
            Vector2f atlas{ 0.0f, 0.0f };
            i32      layer = 0;
        };

    public:
        Sprite            sprite;
        std::vector<Tile> tiles;
        Vector2f          gridSize{ 64.0f, 64.0f };
        Vector2f          tileSize{ 64.0f, 64.0f };
        Vector2f          currentTile{};
        State             currentState = State::Brush;
        i32               currentLayer = 0;

    public:
        void AddTile(const Vector3f pos, const i32 tileId);
        void AddTile(const Vector3f pos, const Vector2f atlas);
        void RemoveTile(const Vector3f pos);
    };

    struct TilesetAnimationComponent : public Component
    {
        CX_COMPONENT

    public:
        struct Animation
        {
            Vector2f startingTile{ 0.0f, 0.0f };
            u32      frameCount = 0;
            f32      frameRate  = 24.0f;
        };

    public:
        Sprite                                     sprite;
        std::unordered_map<std::string, Animation> animations;
    };

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
} // namespace codex

/*
namespace nlohmann {
    template <>
    struct adl_serializer<codex::TilemapComponent::Tile>
    {
        static void to_json(ordered_json& j, const codex::TilemapComponent::Tile& tile)
        {
            j = ordered_json{ { "Position", tile.pos }, { "Atlas", tile.atlas }, { "Layer", tile.layer } };
        }
        static void from_json(const ordered_json& j, codex::TilemapComponent::Tile& tile)
        {
            j.at("Position").get_to(tile.pos);
            j.at("Atlas").get_to(tile.atlas);
            j.at("Layer").get_to(tile.layer);
        }
    };
}; // namespace nlohmann
*/
