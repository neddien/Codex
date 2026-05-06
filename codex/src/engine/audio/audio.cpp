#include "public/audio.h"

#include <fmod_studio.hpp>

namespace codex::ax {
    using namespace FMOD;

    EventHandle::EventHandle() noexcept
        : instance_{}
    {
    }

    EventHandle::EventHandle(Studio::EventInstance* instance) noexcept
        : instance_{ instance }
    {
    }

    EventHandle::EventHandle(EventHandle&& other) noexcept
    {
        if (other.instance_) {
            instance_       = other.instance_;
            other.instance_ = nullptr;
        }
    }

    EventHandle::~EventHandle() noexcept
    {
        if (instance_) {
            instance_->release();
            instance_ = nullptr;
        }
    }

    EventHandle& EventHandle::operator=(EventHandle&& other) noexcept
    {
        return EventHandle{ std::move(other) }.swap(*this);
    }

    void EventHandle::set_parameter(const std::string_view name, const f32 value) noexcept
    {
        if (instance_) {
            // TODO: seek speed?
            instance_->setParameterByName(name.data(), value);
        }
    }

    f32 EventHandle::get_parameter(const std::string_view name) const noexcept
    {
        if (instance_) {
            f32 val;
            instance_->getParameterByName(name.data(), &val);
            return val;
        }

        return .0f;
    }

    void EventHandle::set_volume(const f32 volume) noexcept
    {
        if (instance_) {
            instance_->setVolume(volume);
        }
    }

    void EventHandle::set_pitch(const f32 pitch) noexcept
    {
        if (instance_) {
            instance_->setPitch(pitch);
        }
    }

    void EventHandle::set_min_max_distance(const f32 min, const f32 max) noexcept
    {
        if (instance_) {
            instance_->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MINIMUM_DISTANCE, min);
            instance_->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MAXIMUM_DISTANCE, max);
        }
    }

    void EventHandle::set_spatial_attributes(const SpatialAttributes& attr) noexcept
    {
        if (instance_) {
            FMOD_3D_ATTRIBUTES fattr;
            fattr.position = { attr.position.x, attr.position.y, attr.position.z };
            fattr.forward  = { attr.forward.x, attr.forward.y, attr.forward.z };
            fattr.up       = { attr.up.x, attr.up.y, attr.up.z };
            fattr.velocity = { attr.velocity.x, attr.velocity.y, attr.velocity.z };
            instance_->set3DAttributes(&fattr);
        }
    }

    void EventHandle::set_paused(const bool paused) noexcept
    {
        if (instance_) {
            instance_->setPaused(paused);
        }
    }

    void EventHandle::play() noexcept
    {
        if (instance_) {
            instance_->start();
        }
    }

    void EventHandle::stop(const bool allowFadeout) noexcept
    {
        if (instance_) {
            instance_->stop((allowFadeout) ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE);
        }
    }

    bool EventHandle::is_playing() const noexcept
    {
        if (instance_) {
            FMOD_STUDIO_PLAYBACK_STATE state;
            instance_->getPlaybackState(&state);
            return state != FMOD_STUDIO_PLAYBACK_STOPPED;
        }

        return false;
    }

    EventHandle& EventHandle::swap(EventHandle& other) noexcept
    {
        std::swap(instance_, other.instance_);
        return *this;
    }
} // namespace codex::ax
