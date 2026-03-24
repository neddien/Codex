#include "audio_system.h"

#include <engine/core/common_internal.h>

namespace codex::ax {
    using namespace FMOD;

    static Studio::System* s_system_ = nullptr;

    void AudioSystem::init()
    {
        if (is_valid()) {
            throw AudioInitException("Audio sub-system has already been initialized.");
        }

        if (const auto ret = Studio::System::create(&s_system_); ret != FMOD_OK) {
            throw AudioInitException("Failed to create an FMOD system. Err code: {}", enum_name(ret));
        }

#ifdef CX_BUILD_TYPE_DEBUG
        if (const auto ret =
                s_system_->initialize(DefaultChannelCount, FMOD_STUDIO_INIT_NORMAL | FMOD_STUDIO_INIT_LIVEUPDATE,
                                      FMOD_INIT_NORMAL | FMOD_INIT_PROFILE_ENABLE, nullptr);
            ret != FMOD_OK)
#else
        if (const auto ret =
                s_system_->initialize(DefaultChannelCount, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
            ret != FMOD_OK)
#endif
        {
            throw AudioInitException("Failed to initialize FMOD system. Err code: {}", enum_name(ret));
        }
    }

    void AudioSystem::dispose() noexcept
    {
        if (s_system_) {
            s_system_->release();
            s_system_ = nullptr;
            codex::info("Audio subsystem disposed.");
        }
    }

    Studio::System* AudioSystem::get_fmod_system() noexcept
    {
        return s_system_;
    }

    bool AudioSystem::is_valid() noexcept
    {
        return s_system_;
    }

    void AudioSystem::update()
    {
        if (!is_valid()) {
            throw AudioInitException("Audio subsystem has not been initialized.");
        }

        if (const auto ret = s_system_->update(); ret != FMOD_OK) {
            codex::error("FMOD update failed! Err code: {}", enum_name(ret));
        }
    }

    void AudioSystem::set_listener_attributes(const SpatialAttributes& attr)
    {
        if (!is_valid()) {
            throw AudioInitException("Audio subsystem has not been initialized.");
        }

        FMOD_3D_ATTRIBUTES fattr;
        fattr.position = { attr.position.x, attr.position.y, attr.position.z };
        fattr.forward  = { attr.forward.x, attr.forward.y, attr.forward.z };
        fattr.up       = { attr.up.x, attr.up.y, attr.up.z };
        fattr.velocity = { attr.velocity.x, attr.velocity.y, attr.velocity.z };
        s_system_->setListenerAttributes(0, &fattr);
    }
} // namespace codex::ax
