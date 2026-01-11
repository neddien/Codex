#include "Public/NativeBehaviour.h"

#include <Engine/Scene/Public/Components.inl>

namespace codex {
    TransformComponent& NativeBehaviour::GetTransform() noexcept
    {
        return m_Parent.GetComponent<TransformComponent>();
    }

    void NativeBehaviour::Serialize(ISerializationNode& node) const
    {
    }

    void NativeBehaviour::Deserialize(const ISerializationNode& node)
    {
    }
} // namespace codex
