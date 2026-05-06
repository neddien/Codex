#pragma once

#include <engine/core/public/serializer.h>

namespace codex::phys {
    struct PhysicsMaterial2D : public ISerializable
    {
        f32 density_               = 1.0f;
        f32 friction_              = 0.5f;
        f32 restitution_           = 0.5f;
        f32 restitution_threshold_ = 0.5f;

    public:
        void serialize(ISerializationNode& node) const override
        {
            node.write("density", density_);
            node.write("friction", friction_);
            node.write("restitution", restitution_);
            node.write("restitution_threshold", restitution_threshold_);
        }
        void deserialize(const ISerializationNode& node) override
        {
            node.read("density", density_);
            node.read("friction", friction_);
            node.read("restitution", restitution_);
            node.read("restitution_threshold", restitution_threshold_);
        }
    };
} // namespace codex::phys
