#pragma once

#include <Engine/Core/Public/Exception.h>

// Forward declarations
namespace FMOD::Studio {
    class EventInstance;
} // namespace FMOD::Studio

namespace codex::ax {
    CX_CUSTOM_EXCEPTION(AudioException, "Audio playback failed.");

    struct SpatialAttributes
    {
        Vector3f position{};
        Vector3f velocity{};
        Vector3f forward{ .0f, .0f, 1.0f };
        Vector3f up{ .0f, 1.0f, .0f };
    };

    class CODEX_API SoundHandle
    {
    public:
        void SetVolume(const f32 volume);
        void SetPitch(const f32 pitch);
        void SetPosition(const Vector3& position);
        void SetPaused(const bool paused);
        void Stop();
        bool IsPlaying() const;
    };

    class CODEX_API EventHandle
    {
    private:
        FMOD::Studio::EventInstance* m_Instance;

    public:
        EventHandle() noexcept;
        EventHandle(FMOD::Studio::EventInstance* instance) noexcept;
        EventHandle(const EventHandle&) noexcept = delete;
        EventHandle(EventHandle&& other) noexcept;
        ~EventHandle() noexcept;

    public:
        EventHandle& operator=(const EventHandle&) noexcept = delete;
        EventHandle& operator=(EventHandle&& other) noexcept;

    public:
        void               SetParameter(const std::string_view name, const f32 value) noexcept;
        [[nodiscard]] f32  GetParameter(const std::string_view name) const noexcept;
        void               SetVolume(const f32 volume) noexcept;
        void               SetPitch(const f32 pitch) noexcept;
        void               SetMinMaxDistance(const f32 min, const f32 max) noexcept;
        void               SetSpatialAttributes(const SpatialAttributes& attr) noexcept;
        void               SetPaused(const bool paused) noexcept;
        void               Play() noexcept;
        void               Stop(const bool allowFadeout = false) noexcept;
        [[nodiscard]] bool IsPlaying() const noexcept;
        EventHandle&       Swap(EventHandle& other) noexcept;
    };
} // namespace codex::ax
