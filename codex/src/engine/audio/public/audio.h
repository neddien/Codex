#pragma once

#include <engine/core/public/exception.h>

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
        void set_volume(const f32 volume);
        void set_pitch(const f32 pitch);
        void set_position(const Vector3& position);
        void set_paused(const bool paused);
        void stop();
        bool is_playing() const;
    };

    class CODEX_API EventHandle
    {
    private:
        FMOD::Studio::EventInstance* instance_;

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
        void               set_parameter(const std::string_view name, const f32 value) noexcept;
        [[nodiscard]] f32  get_parameter(const std::string_view name) const noexcept;
        void               set_volume(const f32 volume) noexcept;
        void               set_pitch(const f32 pitch) noexcept;
        void               set_min_max_distance(const f32 min, const f32 max) noexcept;
        void               set_spatial_attributes(const SpatialAttributes& attr) noexcept;
        void               set_paused(const bool paused) noexcept;
        void               play() noexcept;
        void               stop(const bool allowFadeout = false) noexcept;
        [[nodiscard]] bool is_playing() const noexcept;
        EventHandle&       swap(EventHandle& other) noexcept;
    };
} // namespace codex::ax
