#pragma once

#include <sdafx.h>

#include <Engine/Memory/Public/Memory.h>
#include <Engine/Core/Public/UUID.h>

namespace codex {
    class Scene;

    class SceneManager
    {
    private:
        std::unordered_map<UUID, mem::Box<Scene>> m_Scenes;

    public:
        SceneManager() {}
        ~SceneManager() noexcept {}

    public:
        [[nodiscard]] Scene* CreateScene() noexcept;
        [[nodiscard]] Scene* GetSceneFromUUID(const UUID uuid) noexcept;
    };
}