#ifndef CODEX_PHYSICS_MATERIAL_2D_H
#define CODEX_PHYSICS_MATERIAL_2D_H

#include <Engine/Core/Public/Serializer.h>

namespace codex::phys {
    struct PhysicsMaterial2D : public ISerializable
    {
        f32 density              = 1.0f;
        f32 friction             = 0.5f;
        f32 restitution          = 0.5f;
        f32 restitutionThreshold = 0.5f;

    public:
        void Serialize(ISerializationNode& node) const override
        {
            node.Write("Density", density);
            node.Write("Friction", friction);
            node.Write("Restitution", restitution);
            node.Write("RestitutionThreshold", restitutionThreshold);
        }
        void Deserialize(const ISerializationNode& node) override
        {
            node.Read("Density", density);
            node.Read("Friction", friction);
            node.Read("Restitution", restitution);
            node.Read("RestitutionThreshold", restitutionThreshold);
        }
    };
} // namespace codex::phys

#endif // CODEX_PHYSICS_MATERIAL_2D_H
