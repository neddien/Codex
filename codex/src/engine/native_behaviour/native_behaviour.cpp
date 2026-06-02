#include "public/native_behaviour.h"

#include <engine/audio/audio_manager.h>
#include <engine/scene/public/components.inl>

namespace codex {
    TransformComponent& NativeBehaviour::transform() noexcept
    {
        return parent_.get_component<TransformComponent>();
    }

    ax::EventHandle NativeBehaviour::get_audio_event(const std::string_view event_path)
    {
        return ax::AudioManager::load_event(event_path);
    }

    void NativeBehaviour::archive(Archive& ar)
    {
    }
} // namespace codex
