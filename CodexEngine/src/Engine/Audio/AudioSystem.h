#pragma once

#include <sdafx.h>

#include <Engine/Audio/Public/Audio.h>
#include <Engine/Core/Public/Exception.h>

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
        [[nodiscard]] static FMOD::Studio::System* GetFMODSystem() noexcept;

    public:
        [[nodiscard]] static bool IsValid() noexcept;

    public:
        static void Init();
        static void Update();
        static void SetListenerAttributes(const SpatialAttributes& attr);
        static void Dispose() noexcept;
    };
} // namespace codex::ax
