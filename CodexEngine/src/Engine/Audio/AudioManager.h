#pragma once

#include <fmod_studio.hpp>

#include <Engine/Audio/AudioSystem.h>
#include <Engine/Core/Public/Exception.h>

#include "Public/Audio.h"

namespace codex::ax {
    struct EventParameterInfo
    {
        std::string name;
        f32         minimum{};
        f32         maximum{};
        f32         defaultValue{};
    };

    class CODEX_API AudioManager
    {
    public:
        // One-shot playback
        // static void PlaySound(const std::filesystem::path& path);
        // static void PlaySound(const std::filesystem::path& path, float volume);
        // static void PlaySound(const std::filesystem::path& path, const Vector3& position);

        // Controlled playback
        // static SoundHandle Play(const std::filesystem::path& path);

        // Event playback (Studio API) - requires .bank
        static EventHandle              LoadEvent(const std::string_view eventPath);
        static std::future<EventHandle> LoadEventAsync(const std::string_view eventPath);

        // Bank management
        static void              LoadBank(const std::filesystem::path& bankPath);
        static std::future<void> LoadBankAsync(const std::filesystem::path& bankPath);

        // Event enumeration
        static std::vector<std::string>         GetAllEventPaths();
        static std::vector<EventParameterInfo>  GetEventParameters(const std::string_view eventPath);

        // Global controls
        static void SetMasterVolume(const f32 volume);
        static void PauseAll();
        static void ResumeAll();
        static void StopAll();
    };
} // namespace codex::ax
