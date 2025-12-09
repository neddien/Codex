#include "Public/SceneManager.h"

#include <Engine/Scene/Public/Entity.inl>

namespace codex {
    Scene* SceneManager::CreateScene() noexcept
    {
        auto  auto_ptr   = mem::Box<Scene>::New();
        auto* ptr        = auto_ptr.Get();
        m_Scenes[UUID{}] = std::move(auto_ptr);
        return ptr;
    }

    Scene* SceneManager::GetSceneFromUUID(const UUID uuid) noexcept
    {
        if (m_Scenes.contains(uuid)) {
            return m_Scenes[uuid].Get();
        }
        return nullptr;
    }
} // namespace codex