#pragma once

#include <modex.h>

using namespace codex;

RF_CLASS(Category = "Gameplay", DisplayName = "Player Controller")
class CODEX_EXPORT PlayerController : public NativeBehaviour
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
    Vector2f velocity_ = { 20.0f, 20.0f };

    RF_PROPERTY(DisplayName = "Current Velocity", Category = "Movement")
    Vector2f current_velocity_ = { 0.0f, 0.0f };

    RF_PROPERTY(DisplayName = "Subitotus", Category = "Movement")
    Vector3f sub_ = { 10.0f, 10.0f, 0.0f };

    RF_PROPERTY(DisplayName = "Camera Lerp", Category = "Camera")
    f32 lerp_ = 0.5f;

    RF_PROPERTY(DisplayName = "AX Event", Category = "Audio")
    std::string ax_event_ = "event:/Phonograph-music-1";
};
