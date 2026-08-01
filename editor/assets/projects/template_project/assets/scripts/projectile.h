#include <modex.h>

using namespace codex;

RF_CLASS(Category = "Projectiles", DisplayName = "Projectile")
class Projectile : public NativeBehaviour, private Loggable<"Projectile">
{
    RF_SERIALIZABLE

public:
    void on_init() override
    {
        if (!has_component<RigidBody2DComponent>()) {
            throw InvalidOperationException("Projectile requires a RigidBody2DComponent to be attached");
        }

        rb2d_ = &get_component<RigidBody2DComponent>();

        rb2d_->apply_linear_impulse(velocity_);

        ts_ = 0;
    }
    void on_update(f32 dt) override
    {
        if (ts_ >= lifetime_ms_ / 1000.0f) {
            dispose_self();
            ts_ = 0;
        }

        ts_ += dt;
    }
    void on_fixed_update(f32 dt) override {}

private:
    RF_PROPERTY(DisplayName = "Velocity", Category = "Properties")
    vec2 velocity_{ 100.0f, 0.0f };
    f32  lifetime_ms_ = 2000.0f;

private:
    RigidBody2DComponent* rb2d_ = nullptr;
    f32                   ts_   = 0;
};
