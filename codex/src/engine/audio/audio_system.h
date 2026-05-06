#pragma once

#include <engine/audio/public/audio.h>
#include <engine/core/public/exception.h>
#include <engine/core/public/log.h>
#include <engine/memory/public/memory.h>

namespace FMOD {
    namespace Studio {
        class System;
    }
} // namespace FMOD

namespace codex::ax {
    CX_CUSTOM_EXCEPTION(AudioInitException, "Audio sub-system failed to initialize");

    constexpr auto DefaultChannelCount = 512;

    namespace detail {
        void mklog(const LogLevel, const char*);
    }

    class AudioSystem : public Loggable<"AudioSystem">
    {
        friend class Audio;
        friend class AudioManager;
        friend void detail::mklog(const LogLevel, const char*);

    private:
        AudioSystem()  = default;
        ~AudioSystem() = default;

    private:
        [[nodiscard]] static FMOD::Studio::System* get_fmod_system() noexcept;

    public:
        [[nodiscard]] static bool is_valid() noexcept;

    public:
        static AudioSystem& get() noexcept;
        static void         init();
        static void         update();
        static void         set_listener_attributes(const SpatialAttributes& attr);
        static void         dispose() noexcept;

    private:
        FMOD::Studio::System* fmod_sys_ = nullptr;
    };
} // namespace codex::ax
