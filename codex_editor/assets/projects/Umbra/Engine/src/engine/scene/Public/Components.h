#pragma once

#include <engine/audio/public/audio.h>
#include <engine/core/public/uuid.h>
#include <engine/memory/public/memory.h>
#include <engine/physics/public/physics_material2_d.h>

#include "camera.h"
#include "entity.h"
#include "sprite.h"

#define CX_COMPONENT(type)                                                                                             \
    friend class Entity;                                                                                               \
    friend class Scene;                                                                                                \
    friend class ComponentFactory;                                                                                     \
                                                                                                                       \
public:                                                                                                                \
    [[nodiscard]] inline std::string_view type_name() const override                                                   \
    {                                                                                                                  \
        return #type;                                                                                                  \
    }                                                                                                                  \
    [[nodiscard]] mem::Box<Component> clone() const noexcept override                                                  \
    {                                                                                                                  \
        return mem::Box<type>{ new type{ *this } };                                                                    \
    }

namespace codex {
    // Forward declarations
    namespace scene {
        class Prefab;
    } // namespace scene
    class NativeBehaviour;

    struct CODEX_API Component : public ISerializable
    {
        friend class Entity;
        friend class Scene;
        friend class scene::Prefab;

    protected:
        virtual void on_init() {}

    public:
        virtual ~Component() = default;

    public:
        virtual std::string_view    type_name() const      = 0;
        virtual mem::Box<Component> clone() const noexcept = 0;

    public:
        void serialize(ISerializationNode& node) const
        {
            node.write("type", type_name());
            serialize_impl(node);
        }
        void deserialize(const ISerializationNode& node) { deserialize_impl(node); }

    protected:
        virtual void serialize_impl(ISerializationNode& node) const {}
        virtual void deserialize_impl(const ISerializationNode& node) {}

    protected:
        Component* next_ = nullptr;
        Entity     parent_;
    };

    struct CODEX_API IDComponent : public Component
    {
        CX_COMPONENT(IDComponent)

    public:
        UUID uuid;

    public:
        IDComponent() noexcept = default;
        explicit IDComponent(UUID uuid) noexcept
            : uuid(std::move(uuid))
        {
        }

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API TagComponent : public Component
    {
        CX_COMPONENT(TagComponent)

    public:
        std::string tag;

    public:
        TagComponent();
        TagComponent(const std::string_view tag);

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API TransformComponent : public Component
    {
        CX_COMPONENT(TransformComponent)

    public:
        Vector3f position;
        Vector3f rotation;
        Vector3f scale;

    public:
        TransformComponent(const Vector3f position = Vector3f{}, const Vector3f rotation = Vector3f{},
                           const Vector3f scale = Vector3f{ 1.0f, 1.0f, 1.0f });

    public:
        [[nodiscard]] inline Matrix4f to_matrix() const noexcept
        {
            Matrix4f transform_mat = glm::identity<Matrix4f>();
            transform_mat          = glm::translate(transform_mat, position);
            transform_mat          = glm::rotate(transform_mat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            transform_mat          = glm::rotate(transform_mat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            transform_mat          = glm::rotate(transform_mat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            transform_mat          = glm::scale(transform_mat, scale);
            return transform_mat;
        }

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API SpriteRendererComponent : public Component
    {
        CX_COMPONENT(SpriteRendererComponent)

    public:
        SpriteRendererComponent() = default;
        SpriteRendererComponent(Sprite sprite);

    public:
        // TODO: Handle sprite renderers that do not have an actual sprite.
        [[nodiscard]] inline Sprite&       sprite() noexcept { return sprite_; }
        [[nodiscard]] inline const Sprite& sprite() const noexcept { return sprite_; }

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;

    private:
        Sprite sprite_;
    };

    CX_CUSTOM_EXCEPTION(ScriptException, "An unknown behaviour exception occured.")
    CX_CUSTOM_EXCEPTION(DuplicateBehaviourException, "Cannot have more than one type of behaviour on a single entity.")

    struct CODEX_API NativeBehaviourComponent : public Component
    {
        CX_COMPONENT(NativeBehaviourComponent)

    public:
        using BehaviourMap  = std::unordered_map<std::string, mem::Box<NativeBehaviour>>;
        using BehaviourList = std::vector<NativeBehaviour*>;

    public:
        NativeBehaviourComponent() noexcept = default;
        NativeBehaviourComponent(const NativeBehaviourComponent& other);
        NativeBehaviourComponent& operator=(const NativeBehaviourComponent& other);
        NativeBehaviourComponent(NativeBehaviourComponent&& other) noexcept            = default;
        NativeBehaviourComponent& operator=(NativeBehaviourComponent&& other) noexcept = default;
        ~NativeBehaviourComponent() noexcept { dispose_behaviours(); }

    public:
        inline void swap(NativeBehaviourComponent& other) noexcept { std::swap(behaviours_, other.behaviours_); }
        [[nodiscard]] inline BehaviourMap&       behaviours() noexcept { return behaviours_; }
        [[nodiscard]] inline const BehaviourMap& behaviours() const noexcept
        {
            return const_cast<NativeBehaviourComponent*>(this)->behaviours();
        }

    public:
        void                      on_init() override;
        void                      attach(mem::Box<NativeBehaviour> bh);
        mem::Box<NativeBehaviour> detach(const std::string& class_name);
        void                      instantiate_behaviour(const std::string& class_name);
        void                      on_update(const f32 delta_time);
        void                      on_fixed_update(const f32 delta_time);
        void                      dispose_behaviours();
        void                      set_parent(const Entity entity) const noexcept;
        void                      dispose(const std::string& class_name);
        void                      attach_pending_scripts();
        void                      save_attached_to_pending();

    public:
        template <typename T, typename... TArgs>
        T& make_behaviour(TArgs&&... args)
            requires(std::is_base_of_v<NativeBehaviour, T>);

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;

    private:
        mutable BehaviourMap     behaviours_;
        mutable BehaviourList    behaviour_list_; // For iteration.
        std::vector<std::string> pending_scripts_;
    };

    struct CODEX_API CameraComponent : public Component
    {
        CX_COMPONENT(CameraComponent)

    public:
        scene::Camera camera;
        bool          primary = true;
        Vector3f      focalPoint{ 0.0f };

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API RigidBody2DComponent : public Component
    {
        CX_COMPONENT(RigidBody2DComponent)

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
        void apply_force(const Vector2f& force, const std::optional<Vector2f> point = std::nullopt) noexcept;
        void apply_torque(const f32 torque) noexcept;
        void apply_linear_impulse(const Vector2f& impulse, const std::optional<Vector2f> point = std::nullopt);
        void apply_angular_impulse(const f32 torque);

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API BoxCollider2DComponent : public Component
    {
        CX_COMPONENT(BoxCollider2DComponent)

    public:
        Vector2f offset{ 0.0f, 0.0f };
        Vector2f size{ 32.0f, 32.0f };

        phys::PhysicsMaterial2D physicsMaterial;

        void* runtimeFixture = nullptr;

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API CircleCollider2DComponent : public Component
    {
        CX_COMPONENT(CircleCollider2DComponent)

    public:
        Vector2f offset{ 0.0f, 0.0f };
        f32      radius = 32.0f;

        phys::PhysicsMaterial2D physicsMaterial;

        void* runtimeFixture = nullptr;

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API GridRendererComponent : public Component
    {
        CX_COMPONENT(GridRendererComponent)

    public:
        Vector2f cellSize{ 64.0f, 64.0f };
        Vector4f colour{ 1.0f, 1.0f, 1.0f, 0.3f };

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API TilemapComponent : public Component
    {
        CX_COMPONENT(TilemapComponent)

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
        void add_tile(const Vector3f pos, const i32 tileId);
        void add_tile(const Vector3f pos, const Vector2f atlas);
        void remove_tile(const Vector3f pos);

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API TilesetAnimationComponent : public Component
    {
        CX_COMPONENT(TilesetAnimationComponent)

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

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API AudioSourceComponent : public Component
    {
        CX_COMPONENT(AudioSourceComponent)

    public:
        std::string           eventPath;
        std::filesystem::path soundPath;

        f32  volume      = 1.0f;
        f32  pitch       = 1.0f;
        bool loop        = false;
        bool playOnStart = false;
        bool is3D        = true;
        f32  minDistance = 1.0f;
        f32  maxDistance = 100.0f;

        std::unordered_map<std::string, f32> parameters;

        // Runtime state (not serialized)
        mem::Shared<ax::EventHandle> handle;

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct AudioListenerComponent : public Component
    {
        CX_COMPONENT(AudioListenerComponent)

        // Dummy component for now.
        // Position/orientation taken from TransformComponent.
        // Only one active listener at a time.
    };
} // namespace codex
