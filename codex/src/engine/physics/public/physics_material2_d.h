#pragma once

#include <engine/core/public/archive.h>

namespace codex::phys {
    struct PhysicsMaterial2D : public ISerializable
    {
        f32 density_               = 1.0f;
        f32 friction_              = 0.5f;
        f32 restitution_           = 0.5f;
        f32 restitution_threshold_ = 0.5f;

    public:
        void archive(Archive& ar) override
        {
            ar("density", density_);
            ar("friction", friction_);
            ar("restitution", restitution_);
            ar("restitution_threshold", restitution_threshold_);
        }
    };
} // namespace codex::phys
