#include "player_controller.h"

void PlayerController::on_init()
{
    if (has_component<RigidBody2DComponent>())
        rb2d_ = &get_component<RigidBody2DComponent>();
    else
        log(Error, "Doesnt have a rigidbody component");
    camera_ = primary_camera_entity();
}

void PlayerController::on_update(const f32 dt)
{
    static f32 fire_rate_t = 0;
    if (Input::is_key_down(Key::J) && projectile_ && fire_rate_t >= fire_rate_) {
        create_prefab(projectile_, get_component<TransformComponent>());
        fire_rate_t = 0;
    }

    fire_rate_t += dt;
}

void PlayerController::on_fixed_update(const f32 dt)
{
    if (camera_ && rb2d_) {
        auto& camera    = camera_.get_component<CameraComponent>();
        auto& cam_tc    = camera_.get_component<TransformComponent>();
        auto& tc        = transform();
        auto  lerp_pos  = glm::mix(tc.position, cam_tc.position, lerp_);
        cam_tc.position = lerp_pos - sub_;

        current_velocity_ = { 0.0f, 0.0f };
        if (Input::is_key_down(Key::W))
            current_velocity_.y = velocity_.y;
        if (Input::is_key_down(Key::A))
            current_velocity_.x = velocity_.x * -1;
        if (Input::is_key_down(Key::S))
            current_velocity_.y = velocity_.y * -1;
        if (Input::is_key_down(Key::D))
            current_velocity_.x = velocity_.x;
        if (Input::is_key_down(Key::Left))
            rb2d_->apply_torque(1.0f);
        if (Input::is_key_down(Key::Right))
            rb2d_->apply_torque(-1.0f);
        if (Input::is_key_down(Key::F))
            throw CodexException("Why did you press F?");

        rb2d_->apply_force(current_velocity_);
    }
}
