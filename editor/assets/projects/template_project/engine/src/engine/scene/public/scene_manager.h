#pragma once

#include <engine/core/public/uuid.h>
#include <engine/memory/public/memory.h>

namespace codex {
    class Scene;

    class SceneManager : public Loggable<"SceneManager">
    {
    public:
        SceneManager() {}
        ~SceneManager() noexcept {}

    public:
        [[nodiscard]] Scene* create_scene() noexcept;
        [[nodiscard]] Scene* scene_from_uuid(const UUID uuid) noexcept;

    private:
        std::unordered_map<UUID, Box<Scene>> scenes_;
    };
} // namespace codex
