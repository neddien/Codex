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
            node.write("Density", density_);
            node.write("Friction", friction_);
            node.write("Restitution", restitution_);
            node.write("RestitutionThreshold", restitution_threshold_);
        }
        void deserialize(const ISerializationNode& node) override
        {
            node.read("Density", density_);
            node.read("Friction", friction_);
            node.read("Restitution", restitution_);
            node.read("RestitutionThreshold", restitution_threshold_);
        }
    };
} // namespace codex::phys
