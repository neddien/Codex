#include "TestScript.h"

void MienScripten::OnInit()
{
}

void MienScripten::OnUpdate(const f32 deltaTime)
{
    if (m_Move)
    {
        static float count     = 0;
        static auto& transform = GetTransform();
        Vector3f     temp      = (transform.position + std::sin(count)) * m_Axies * m_Multiplier * deltaTime;
        transform.position += temp;
        count += 1.0f;
    }
}

void MienScripten::OnFixedUpdate(const f32 deltaTime)
{
	using clock = std::chrono::high_resolution_clock;
	
	static auto tp1 = clock::now();
	
	const auto tp2 = clock::now();
	const auto frame_time = std::chrono::duration<float>(tp2 - tp1).count();
	tp1 = tp2;
	
	fmt::println("Current Tick Rate: {} Intended Tick Rate: {}", 1.0f / frame_time, 1.0f / deltaTime);
}
