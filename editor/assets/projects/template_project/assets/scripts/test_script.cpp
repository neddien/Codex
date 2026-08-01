#include "test_script.h"

void MienScripten::on_init()
{
}

void MienScripten::on_update(const f32 delta_time)
{
    if (move_) {
        static float count = 0;
        static auto& tc    = transform();
        vec3     temp  = (tc.position + std::sin(count)) * axies_ * multiplier_ * delta_time;
        tc.position += temp;
        count += 1.0f;
    }
}

void MienScripten::on_fixed_update(const f32 delta_time)
{
    using clock = std::chrono::high_resolution_clock;

    static auto tp1 = clock::now();

    const auto tp2        = clock::now();
    const auto frame_time = std::chrono::duration<float>(tp2 - tp1).count();
    tp1                   = tp2;

    fmt::println("Current Tick Rate: {} Intended Tick Rate: {}", 1.0f / frame_time, 1.0f / delta_time);
}
