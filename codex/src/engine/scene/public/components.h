#pragma once

#include <engine/audio/public/audio.h>
#include <engine/core/public/uuid.h>
#include <engine/memory/public/memory.h>
#include <engine/native_behaviour/public/native_behaviour_manager.h>
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
    [[nodiscard]] std::string_view type_name() const noexcept override                                                 \
    {                                                                                                                  \
        return #type;                                                                                                  \
    }

namespace codex {
    // Forward declarations
    namespace scene {
        class Prefab;
    } // namespace scene
    class NativeBehaviour;

    class CODEX_API Component : public ISerializable
    {
        friend class Entity;
        friend class Scene;
        friend class scene::Prefab;

    protected:
        virtual void on_init() {}

    public:
        virtual ~Component() = default;

    public:
        virtual std::string_view type_name() const noexcept = 0;

    public:
        void serialize(ISerializationNode& node) const
        {
            node.write("type", type_name());
            serialize_impl(node);
        }
        void deserialize(const ISerializationNode& node)
        {
            /* clang-format: no inline */
            deserialize_impl(node);
        }

    protected:
        virtual void serialize_impl(ISerializationNode& node) const {}
        virtual void deserialize_impl(const ISerializationNode& node) {}

    protected:
        mutable Component* next_ = nullptr;
        mutable Entity     parent_;
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
        explicit TagComponent(const std::string_view tag);

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
        explicit TransformComponent(const Vector3f position = Vector3f{}, const Vector3f rotation = Vector3f{},
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

    class CODEX_API SpriteRendererComponent : public Component
    {
        CX_COMPONENT(SpriteRendererComponent)

    public:
        SpriteRendererComponent() = default;
        explicit SpriteRendererComponent(Sprite sprite);

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

    class NativeBehaviourComponent : public Component, public Loggable<"NativeBehaviourComponent">
    {
        CX_COMPONENT(NativeBehaviourComponent)

    public:
        NativeBehaviourComponent() noexcept;
        NativeBehaviourComponent(const NativeBehaviourComponent& other) noexcept            = delete;
        NativeBehaviourComponent& operator=(const NativeBehaviourComponent& other) noexcept = delete;
        NativeBehaviourComponent(NativeBehaviourComponent&& other) noexcept;
        NativeBehaviourComponent& operator=(NativeBehaviourComponent&& other) noexcept;
        ~NativeBehaviourComponent() noexcept;

    public:
        void                          on_init() override;
        NativeBehaviour*              attach(const std::string_view type_name) noexcept;
        void                          detach(const std::string_view type_name) noexcept;
        void                          dispose_behaviours() noexcept;
        NativeBehaviour*              behaviour(const std::string_view type_name) noexcept;
        std::vector<NativeBehaviour*> behaviours() noexcept;
        void                          dispose() noexcept;

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;

    private:
        [[nodiscard]] Scene* scene() const noexcept { return parent_.scene(); }
        void                 attach_pending() noexcept;

    private:
        Scene::BagHandle                         handle_ = Scene::NBHandle::invalid_id();
        mutable absl::flat_hash_set<std::string> pending_;
    };

    struct CODEX_API CameraComponent : public Component
    {
        CX_COMPONENT(CameraComponent)

    public:
        scene::Camera camera;
        bool          primary = true;
        Vector3f      focal_point{ 0.0f };

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
        };

    public:
        BodyType body_type       = BodyType::Static;
        bool     fixed_rotation  = false;
        f32      linear_damping  = 0.0f;
        f32      angular_damping = 0.1f;
        bool     high_velocity   = false;
        bool     enabled         = true;
        f32      gravity_scale   = 1.0f;
        void*    runtime_body    = nullptr;

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
        Vector2f                offset{ 0.0f, 0.0f };
        Vector2f                size{ 32.0f, 32.0f };
        phys::PhysicsMaterial2D physics_material;
        void*                   runtime_fixture = nullptr;

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API CircleCollider2DComponent : public Component
    {
        CX_COMPONENT(CircleCollider2DComponent)

    public:
        Vector2f                offset{ 0.0f, 0.0f };
        f32                     radius = 32.0f;
        phys::PhysicsMaterial2D physics_material;
        void*                   runtime_fixture = nullptr;

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API GridRendererComponent : public Component
    {
        CX_COMPONENT(GridRendererComponent)

    public:
        Vector2f cell_size{ 64.0f, 64.0f };
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
        Vector2f          grid_size{ 64.0f, 64.0f };
        Vector2f          tile_size{ 64.0f, 64.0f };
        Vector2f          current_tile{};
        State             current_state = State::Brush;
        i32               current_layer = 0;

    public:
        void add_tile(const Vector3f pos, const i32 tile_id);
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
            std::string name;
            Vector2f    starting_tile{ 0.0f, 0.0f };
            u32         frame_count   = 0;
            f32         frame_rate    = 24.0f;
            u32         current_frame = 0;
        };

    public:
        Sprite                 sprite;
        Vector2f               grid_size{ 32.0f, 32.0f };
        std::vector<Animation> animations;

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct CODEX_API AudioSourceComponent : public Component
    {
        CX_COMPONENT(AudioSourceComponent)

    public:
        std::string                          event_path;
        std::filesystem::path                sound_path;
        f32                                  volume        = 1.0f;
        f32                                  pitch         = 1.0f;
        bool                                 loop          = false;
        bool                                 play_on_start = false;
        bool                                 is_3d         = true;
        f32                                  min_distance  = 1.0f;
        f32                                  max_distance  = 100.0f;
        std::unordered_map<std::string, f32> parameters;
        Shared<ax::EventHandle>              handle;

    public:
        void serialize_impl(ISerializationNode& node) const override;
        void deserialize_impl(const ISerializationNode& node) override;
    };

    struct AudioListenerComponent : public Component
    {
        CX_COMPONENT(AudioListenerComponent)
    };
} // namespace codex
