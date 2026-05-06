#pragma once

#include <modex.h>

using namespace codex;

RF_CLASS(Category = "Gameplay", DisplayName = "Phonograph Controller")
class CODEX_EXPORT PhonographController : public NativeBehaviour
{
    RF_SERIALIZABLE

public:
    void on_init() override;
    void on_update(const f32 delta_time) override;
    void on_fixed_update(const f32 delta_time) override;

private:
    AudioSourceComponent* asc_    = nullptr;
    Entity                camera_ = Entity::none();
};
