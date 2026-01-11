#pragma once

#include <Modex.h>

using namespace codex;

RF_CLASS(Category="Gameplay", DisplayName="Player Controller")
class CODEX_EXPORT PlayerController : public NativeBehaviour
{
    RF_SERIALIZABLE

private:
    RigidBody2DComponent* m_Rb2d   = nullptr;
    Entity                m_Camera = Entity::None();

    RF_PROPERTY(DisplayName="Velocity", Category="Movement")
    Vector2f m_Velocity = { 20.0f, 20.0f };

    RF_PROPERTY(DisplayName="Current Velocity", Category="Movement")
    Vector2f m_CurrentVelocity = { 0.0f, 0.0f };

    RF_PROPERTY(DisplayName="Subitotus", Category="Movement")
    Vector3f m_Sub = { 10.0f, 10.0f, 0.0f };

    RF_PROPERTY(DisplayName="Camera Lerp", Category="Camera")
    f32 m_Lerp = 0.5f;

public:
    void OnInit() override;
    void OnUpdate(const f32 deltaTime) override;
    void OnFixedUpdate(const f32 deltaTime) override;
};
