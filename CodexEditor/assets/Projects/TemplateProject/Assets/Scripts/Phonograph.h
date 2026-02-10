#pragma once

#include <Modex.h>

using namespace codex;

RF_CLASS(Category = "Gameplay", DisplayName = "Phonograph Controller")
class CODEX_EXPORT PhonographController : public NativeBehaviour
{
    RF_SERIALIZABLE

private:
    AudioSourceComponent* m_Asc    = nullptr;
    Entity                m_Camera = Entity::None();

public:
    void OnInit() override;
    void OnUpdate(const f32 deltaTime) override;
    void OnFixedUpdate(const f32 deltaTime) override;
};
