#include "Public/NativeBehaviour.h"

#include <Engine/Audio/AudioManager.h>
#include <Engine/Scene/Public/Components.inl>

namespace codex {
    TransformComponent& NativeBehaviour::GetTransform() noexcept
    {
        return m_Parent.GetComponent<TransformComponent>();
    }

    ax::EventHandle NativeBehaviour::GetAudioEvent(const std::string_view eventPath)
    {
        return ax::AudioManager::LoadEvent(eventPath);
    }

    void NativeBehaviour::Serialize(ISerializationNode& node) const
    {
    }

    void NativeBehaviour::Deserialize(const ISerializationNode& node)
    {
    }
} // namespace codex
