#include "public/scene_manager.h"

#include <box2d/box2d.h>
#include <engine/scene/public/entity.inl>

namespace codex {
    Scene* SceneManager::create_scene() noexcept
    {
        auto  auto_ptr  = Box<Scene>::make();
        auto* ptr       = auto_ptr.get();
        scenes_[UUID{}] = std::move(auto_ptr);
        return ptr;
    }

    Scene* SceneManager::scene_from_uuid(const UUID uuid) noexcept
    {
        if (scenes_.contains(uuid))
            return scenes_[uuid].get();
        return nullptr;
    }
} // namespace codex
