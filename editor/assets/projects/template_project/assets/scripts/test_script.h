#pragma once

#include <modex.h>

using namespace codex;

RF_CLASS(Category = "Gameplay", DisplayName = "asd")
class CODEX_EXPORT MienScripten : public NativeBehaviour
{
    RF_SERIALIZABLE

public:
    void on_init() override;
    void on_update(const f32 delta_time) override;
    void on_fixed_update(const f32 delta_time) override;

private:
    std::string str_;
    RF_PROPERTY()
    bool move_ = false;
    RF_PROPERTY()
    f32 multiplier_ = 1.0f;
    RF_PROPERTY()
    vec3 axies_ = { 0.0f, 1.0f, 0.0f };
};
