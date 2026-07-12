#pragma once

#include <modex.h>

using namespace codex;

RF_CLASS(Category = "Gameplay", DisplayName = "Player Controller")
class CODEX_EXPORT PlayerController : public NativeBehaviour, public Loggable<"PlayerController">
{
    RF_SERIALIZABLE

public:
    void on_init() override;
    void on_update(const f32 delta_time) override;
    void on_fixed_update(const f32 delta_time) override;

private:
    RigidBody2DComponent* rb2d_   = nullptr;
    Entity                camera_ = Entity::none();

    RF_PROPERTY(DisplayName = "Velocity", Category = "Movement")
    vec2 velocity_ = { 150.0f, 150.0f };

    RF_PROPERTY(DisplayName = "Current Velocity", Category = "Movement")
    vec2 current_velocity_ = { 0.0f, 0.0f };

    RF_PROPERTY(DisplayName = "Subitotus", Category = "Movement")
    vec3 sub_ = { 10.0f, 10.0f, 0.0f };

    RF_PROPERTY(DisplayName = "Camera Lerp", Category = "Camera")
    f32 lerp_ = 0.5f;

    RF_PROPERTY(DisplayName = "Fire Rate", Category = "Shooting")
    f32 fire_rate_ = 0.5f;

    RF_PROPERTY(DisplayName = "Projectile Prefab", Category = "Shooting")
    Asset<scene::Prefab> projectile_;
};
