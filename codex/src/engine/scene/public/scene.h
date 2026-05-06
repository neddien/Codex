#pragma once

#include <engine/concurrency/public/mutex.h>
#include <engine/core/public/serializer.h>
#include <engine/memory/public/memory.h>
#include <engine/scene/public/entity.h>
#include <engine/scene/public/prefab.h>

#include <entt.hpp>

// Forward declarations
class b2World;

namespace codex {
    struct B2WorldDeleter
    {
        void operator()(b2World* world) noexcept;
    };
} // namespace codex

namespace codex {
    // Forward declarations
    class Window;
    class Entity;
    class NativeBehaviour;
    namespace scene {
        class EditorCamera;
    } // namespace scene

    class CODEX_API Scene : public ISerializable, public Loggable<"Scene">
    {
        friend class Window;
        friend class Entity;
        friend class Serializer;
        friend class scene::Prefab;

    public:
        struct PhysicsProperties
        {
            i32      tick_rate           = 60;
            u32      velocity_iterations = 6;
            u32      position_iterations = 2;
            f32      scaling_factor      = 1.0f / 64.0f;
            Vector2f gravity             = { 0.0f, -9.8f };
        };
        enum class State
        {
            Play,
            Edit,
            Simulate
        };

    public:
        Scene() noexcept                        = default;
        Scene(const Scene&) noexcept            = delete;
        Scene& operator=(const Scene&) noexcept = delete;
        Scene(Scene&& other) noexcept;
        Scene& operator=(Scene&& other) noexcept;
        ~Scene() noexcept;

    public:
        [[nodiscard]] inline State                    state() const noexcept { return state_.load(); }
        [[nodiscard]] inline PhysicsProperties&       physics_properties() noexcept { return physics_properties_; }
        [[nodiscard]] inline const PhysicsProperties& physics_properties() const noexcept
        {
            return physics_properties_;
        }
        [[nodiscard]] inline std::string_view name() const noexcept { return name_; }
        [[nodiscard]] u32                     entity_count() const noexcept;
        [[nodiscard]] bool                    is_valid(const Entity entity) const noexcept;
        [[nodiscard]] std::vector<Entity>     get_all_entities_with_tag(const std::string_view tag);
        [[nodiscard]] std::vector<Entity>     get_all_entities();
        [[nodiscard]] Entity                  primary_camera_entity() noexcept;

    public:
        inline void set_state(const State new_state) noexcept { state_.store(new_state); }
        void        swap(Scene& other) noexcept;

        template <typename T>
        [[nodiscard]] std::vector<Entity> get_all_entities_with_component() noexcept
        {
            auto                view = registry_->view<T>();
            std::vector<Entity> entities;
            entities.reserve(view.size());
            for (auto& e : view)
                entities.emplace_back(e, this);
            return entities;
        }

        void   copy_to(Scene& other) const noexcept;
        Entity create_entity(const std::string_view tag = "default tag", UUID uuid = UUID{}) noexcept;
        void   remove_entity(const Entity entity);
        void   remove_entity(const u32 entity);
        Entity instantiate_prefab(const scene::Prefab& prefab) noexcept;

        void on_editor_init(scene::EditorCamera& camera);
        void on_runtime_start();
        void on_runtime_stop();
        void on_simulation_start();
        void on_simulation_stop();
        void on_editor_update(const f32 delta_time, scene::EditorCamera& camera);
        void on_runtime_update(const f32 delta_time);
        void on_simulation_update(const f32 delta_time, scene::EditorCamera& camera);

        void serialize(ISerializationNode& node) const override;
        void deserialize(const ISerializationNode& node) override;

    private:
        void        render_sprites();
        void        render_audio();
        void        construct_physics_bodies();
        static void on_fixed_update(Scene& self) noexcept;

    private:
        cc::Mutex<entt::registry>    registry_;
        std::string                  name_          = "Default scene";
        Box<b2World, B2WorldDeleter> physics_world_ = nullptr;
        std::atomic<State>           state_         = State::Edit;
        std::thread                  fixed_update_thread_;
        PhysicsProperties            physics_properties_;
        Box<Entity>                  primary_camera_entity_ = nullptr;
    };
} // namespace codex
