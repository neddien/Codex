#pragma once

#include <Modex.h>

using namespace codex;

RF_CLASS()
class CODEX_EXPORT MienScripten : public NativeBehaviour
{
    RF_SERIALIZABLE

private:
    std::string m_Str;
    RF_PROPERTY()
    bool m_Move = false;
    RF_PROPERTY()
    f32 m_Multiplier = 1.0f;
    RF_PROPERTY()
    Vector3f m_Axies = { 0.0f, 1.0f, 0.0f };

public:
    void OnInit() override;
    void OnUpdate(const f32 deltaTime) override;
	void OnFixedUpdate(const f32 deltaTime) override;
};
