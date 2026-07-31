#pragma once

#include <future>

#include <fmod_studio.hpp>

#include <engine/audio/audio_system.h>
#include <engine/core/public/exception.h>

#include "public/audio.h"

namespace codex::ax {
    struct EventParameterInfo
    {
        std::string name;
        f32         minimum{};
        f32         maximum{};
        f32         default_value{};
    };

    class CODEX_API AudioManager
    {
    public:
        // One-shot playback
        // static void PlaySound(const std::filesystem::path& path);
        // static void PlaySound(const std::filesystem::path& path, float volume);
        // static void PlaySound(const std::filesystem::path& path, const vec3& position);

        // Controlled playback
        // static SoundHandle Play(const std::filesystem::path& path);

        static EventHandle              load_event(const std::string_view event_path);
        static std::future<EventHandle> load_event_async(const std::string_view event_path);

        // Bank management
        static void              load_bank(const std::filesystem::path& bank_path);
        static std::future<void> load_bank_async(const std::filesystem::path& bank_path);
        static void              unload_bank(const std::filesystem::path& bank_path);
        static void              unload_all();

        // Event enumeration
        static std::vector<std::string>        get_all_event_paths();
        static std::vector<EventParameterInfo> event_parameters(const std::string_view eventPath);

        // Global controls
        static void set_master_volume(const f32 volume);
        static void pause_all();
        static void resume_all();
        static void stop_all();
    };
} // namespace codex::ax
