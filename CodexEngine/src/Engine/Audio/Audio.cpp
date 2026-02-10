#include "Public/Audio.h"

#include <fmod_studio.hpp>

namespace codex::ax {
    using namespace FMOD;

    EventHandle::EventHandle() noexcept
        : m_Instance{}
    {
    }

    EventHandle::EventHandle(Studio::EventInstance* instance) noexcept
        : m_Instance{ instance }
    {
    }

    EventHandle::EventHandle(EventHandle&& other) noexcept
    {
        if (other.m_Instance)
        {
            m_Instance       = other.m_Instance;
            other.m_Instance = nullptr;
        }
    }

    EventHandle::~EventHandle() noexcept
    {
        if (m_Instance)
        {
            m_Instance->release();
            m_Instance = nullptr;
        }
    }

    EventHandle& EventHandle::operator=(EventHandle&& other) noexcept
    {
        return EventHandle{ std::move(other) }.Swap(*this);
    }

    void EventHandle::SetParameter(const std::string_view name, const f32 value) noexcept
    {
        if (m_Instance)
        {
            // TODO: seek speed?
            m_Instance->setParameterByName(name.data(), value);
        }
    }

    f32 EventHandle::GetParameter(const std::string_view name) const noexcept
    {
        if (m_Instance)
        {
            f32 val;
            m_Instance->getParameterByName(name.data(), &val);
            return val;
        }

        return .0f;
    }

    void EventHandle::SetVolume(const f32 volume) noexcept
    {
        if (m_Instance)
        {
            m_Instance->setVolume(volume);
        }
    }

    void EventHandle::SetPitch(const f32 pitch) noexcept
    {
        if (m_Instance)
        {
            m_Instance->setPitch(pitch);
        }
    }

    void EventHandle::SetMinMaxDistance(const f32 min, const f32 max) noexcept
    {
        if (m_Instance)
        {
            m_Instance->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MINIMUM_DISTANCE, min);
            m_Instance->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MAXIMUM_DISTANCE, max);
        }
    }

    void EventHandle::SetSpatialAttributes(const SpatialAttributes& attr) noexcept
    {
        if (m_Instance)
        {
            FMOD_3D_ATTRIBUTES fattr;
            fattr.position = { attr.position.x, attr.position.y, attr.position.z };
            fattr.forward  = { attr.forward.x, attr.forward.y, attr.forward.z };
            fattr.up       = { attr.up.x, attr.up.y, attr.up.z };
            fattr.velocity = { attr.velocity.x, attr.velocity.y, attr.velocity.z };
            m_Instance->set3DAttributes(&fattr);
        }
    }

    void EventHandle::SetPaused(const bool paused) noexcept
    {
        if (m_Instance)
        {
            m_Instance->setPaused(paused);
        }
    }

    void EventHandle::Play() noexcept
    {
        if (m_Instance)
        {
            m_Instance->start();
        }
    }

    void EventHandle::Stop(const bool allowFadeout) noexcept
    {
        if (m_Instance)
        {
            m_Instance->stop((allowFadeout) ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE);
        }
    }

    bool EventHandle::IsPlaying() const noexcept
    {
        if (m_Instance)
        {
            FMOD_STUDIO_PLAYBACK_STATE state;
            m_Instance->getPlaybackState(&state);
            return state != FMOD_STUDIO_PLAYBACK_STOPPED;
        }

        return false;
    }

    EventHandle& EventHandle::Swap(EventHandle& other) noexcept
    {
        std::swap(m_Instance, other.m_Instance);
        return *this;
    }
} // namespace codex::ax
