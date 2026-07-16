#include "public/native_behaviour.h"

#include <engine/audio/audio_manager.h>
#include <engine/scene/public/components.inl>
#include <engine/scene/public/entity.inl>

namespace codex {
    TransformComponent& NativeBehaviour::transform() noexcept
    {
        return parent_.get_component<TransformComponent>();
    }

    Entity NativeBehaviour::create_entity(const std::optional<math::transform>& transform, std::string_view tag,
                                          UUID uuid)
    {
        if (parent_)
            return parent_.scene_->create_entity(transform, tag, uuid);
        return {};
    }

    Entity NativeBehaviour::create_prefab(const scene::Prefab& prefab, const std::optional<math::transform>& transform,
                                          std::string_view tag, UUID uuid)
    {
        if (parent_)
            return parent_.scene_->instantiate_prefab(prefab, transform, tag, uuid);
        return {};
    }

    Entity NativeBehaviour::create_prefab(const Asset<scene::Prefab>&           prefab,
                                          const std::optional<math::transform>& transform, std::string_view tag,
                                          UUID uuid)
    {
        if (parent_ && prefab)
            return parent_.scene_->instantiate_prefab(*prefab, transform, tag, uuid);
        return {};
    }

    void NativeBehaviour::remove_entity(Entity entity)
    {
        if (parent_)
            parent_.scene_->remove_entity(entity);
    }

    std::vector<Entity> NativeBehaviour::entities_with_tag(const std::string_view tag)
    {
        if (parent_)
            return parent_.scene_->entities_with_tag(tag);
        return {};
    }

    ax::EventHandle NativeBehaviour::audio_event(const std::string_view event_path)
    {
        return ax::AudioManager::load_event(event_path);
    }

    void NativeBehaviour::dispose_self()
    {
        if (parent_)
            parent_.scene_->enqueue_for_disposal(parent_);
    }

    void NativeBehaviour::archive([[maybe_unused]] Archive& ar)
    {
    }
} // namespace codex
