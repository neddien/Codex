#pragma once

#include <engine/audio/public/audio.h>
#include <engine/core/public/exception.h>

#include <fmod_studio.hpp>

namespace codex::ax {
    CX_CUSTOM_EXCEPTION(AudioInitException, "Audio sub-system failed to initialize");

    constexpr auto DefaultChannelCount = 512;

    class AudioSystem
    {
        friend class Audio;
        friend class AudioManager;

    private:
        AudioSystem()  = default;
        ~AudioSystem() = default;

    private:
        [[nodiscard]] static FMOD::Studio::System* get_fmod_system() noexcept;

    public:
        [[nodiscard]] static bool is_valid() noexcept;

    public:
        static void init();
        static void update();
        static void set_listener_attributes(const SpatialAttributes& attr);
        static void dispose() noexcept;
    };
} // namespace codex::ax
