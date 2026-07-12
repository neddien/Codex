#pragma once

#include <engine/algorithm/public/dense_vector.h>
#include <engine/asset_manager/public/asset_common.h>
#include <engine/concurrency/public/mutex.h>
#include <engine/core/public/archive.h>
#include <engine/core/public/common_third_party_libs.h>
#include <engine/core/public/geometry.h>
#include <engine/graphics/public/shader.h>
#include <engine/memory/public/memory.h>
#include <engine/scene/public/entity.h>

// Forward declarations
class b2World;

namespace codex {
    struct B2WorldDeleter
    {
        void operator()(b2World* world) noexcept;
    };

    namespace scene {
        class Prefab;
    } // namespace scene
} // namespace codex

namespace codex {
    // Forward declarations
    class Window;
    class Entity;
    class NativeBehaviour;

    namespace scene {
        class EditorCamera;
    } // namespace scene

    class CODEX_API Scene : public ISerializable, public Loggable<"Scene">, public IAsset
    {
        CX_ASSET(Scene)

        friend class Window;
        friend class Entity;
        friend class Serializer;
        friend class scene::Prefab;

    public:
        struct NBCRecord;
        struct ImportSettings : public IAssetImportSettings
        {
            CX_ASSET_IMPORT_SETTINGS(ImportSettings)

        public:
            enum ArType
            {
                kJson,
                kBinary,
            };

        public:
            ImportSettings() noexcept
                : ar_type_{ ArType::kJson }
            {
            }
            [[nodiscard]] ArType ar_type() const noexcept { return ar_type_; }
            void                 archive(Archive& ar) override;

        private:
            ArType ar_type_;
        };

    public:
        using BehaviourList = dense_vector<Box<NativeBehaviour>>;
        using NBHandle      = BehaviourList::id_type;
        using BehaviourBag  = dense_vector<NBCRecord>;
        using BagHandle     = BehaviourBag::id_type;

    public:
        struct NBCRecord
        {
            Entity                                             owner;
            absl::flat_hash_set<NBHandle, std::hash<NBHandle>> behaviours;
        };
        struct PhysicsProperties
        {
            i32  tick_rate           = 60;
            u32  velocity_iterations = 6;
            u32  position_iterations = 2;
            f32  scaling_factor      = 1.0f / 64.0f;
            vec2 gravity             = { 0.0f, -9.8f };
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
        { return physics_properties_; }
        [[nodiscard]] inline std::string_view name() const noexcept { return name_; }
        [[nodiscard]] u32                     entity_count() const noexcept;
        [[nodiscard]] bool                    is_valid(const Entity entity) const noexcept;
        [[nodiscard]] std::vector<Entity>     entities_with_tag(const std::string_view tag);
        [[nodiscard]] std::vector<Entity>     entities();
        [[nodiscard]] Entity                  primary_camera_entity() noexcept;

    public:
        inline void set_state(const State new_state) noexcept { state_.store(new_state); }
        void        swap(Scene& other) noexcept;

        template <typename T>
        [[nodiscard]] std::vector<Entity> entities_with_component() noexcept
        {
            auto                view = registry_->view<T>();
            std::vector<Entity> entities;
            entities.reserve(view.size());
            for (auto& e : view)
                entities.emplace_back(e, this);
            return entities;
        }

        void   copy_to(Scene& other) const noexcept;
        void   clone_via_serialization(Scene& other) const;
        Entity create_entity(std::string_view tag) noexcept;
        Entity create_entity(const std::optional<math::transform>& transform = std::nullopt,
                             const std::string_view tag = "default tag", UUID uuid = UUID{}) noexcept;
        void   remove_entity(Entity entity);
        void   remove_entity(u32 entity);
        Entity instantiate_prefab(const scene::Prefab&                  prefab,
                                  const std::optional<math::transform>& transform = std::nullopt,
                                  std::string_view tag = "default tag", UUID uuid = UUID{}) noexcept;
        void   enqueue_for_disposal(Entity entity) noexcept;
        Entity entity_by_uuid(UUID uuid) noexcept;
        void   set_parent(Entity parent, Entity child) noexcept;

        [[nodiscard]] BagHandle             create_behaviour_bag(Entity owner) noexcept;
        void                                dispose_behaviour_bag(BagHandle handle) noexcept;
        [[nodiscard]] NativeBehaviour*      behaviour(NBHandle bhhandle) noexcept;
        [[nodiscard]] std::vector<NBHandle> behaviours(BagHandle handle) noexcept;
        [[nodiscard]] NBHandle              create_behaviour(BagHandle handle, const std::string_view type_name);
        void                                dispose_behaviour(BagHandle handle, NBHandle bhhandle) noexcept;

        void on_editor_init(scene::EditorCamera& camera);
        void on_runtime_start();
        void on_runtime_stop();
        void on_simulation_start();
        void on_simulation_stop();
        void on_editor_update(const f32 delta_time, scene::EditorCamera& camera, gfx::Shader* end_shader = nullptr);
        void on_runtime_update(const f32 delta_time);
        void on_simulation_update(const f32 delta_time, scene::EditorCamera& camera);

        void archive(Archive& archive) override;

    public:
        void attach_pending_behaviours() noexcept;

    private:
        void        transform_pass();
        void        render_sprites();
        void        render_audio();
        void        construct_physics_bodies();
        void        construct_physics_body(entt::registry& registry, const entt::entity entity);
        void        destroy_physics_body(entt::registry& registry, entt::entity entity) noexcept;
        static void on_fixed_update(Scene& self) noexcept;

    private:
        BehaviourBag                      bag_;
        BehaviourList                     behaviours_;
        cc::Mutex<entt::registry>         registry_;
        std::string                       name_          = "Default scene";
        Box<b2World, B2WorldDeleter>      physics_world_ = nullptr;
        std::atomic<State>                state_         = State::Edit;
        std::thread                       fixed_update_thread_;
        PhysicsProperties                 physics_properties_;
        Box<Entity>                       primary_camera_entity_ = nullptr;
        std::vector<Entity>               entities_to_be_disposed_;
        absl::flat_hash_map<UUID, Entity> uuid_to_entity_;
        // Recursive: behaviour callbacks run under this lock and may re-enter
        // scene methods that lock it again (spawning, disposal, ...).
        mutable std::recursive_mutex mutex_;
    };

    void serialize(Archive& ar, Entity& entity);
} // namespace codex
