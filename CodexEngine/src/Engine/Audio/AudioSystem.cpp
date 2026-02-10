#include "AudioSystem.h"

#include <Engine/Core/CommonInternal.h>

namespace codex::ax {
    using namespace FMOD;

    static Studio::System* s_System = nullptr;

    void AudioSystem::Init()
    {
        if (IsValid())
        {
            cx_throw(AudioInitException, "Audio sub-system has already been initialized.");
        }

        if (const auto ret = Studio::System::create(&s_System); ret != FMOD_OK)
        {
            cx_throw(AudioInitException, "Failed to create an FMOD system. Err code: {}", EnumName(ret));
        }

#ifdef CX_BUILD_TYPE_DEBUG
        if (const auto ret =
                s_System->initialize(DefaultChannelCount, FMOD_STUDIO_INIT_NORMAL | FMOD_STUDIO_INIT_LIVEUPDATE,
                                     FMOD_INIT_NORMAL | FMOD_INIT_PROFILE_ENABLE, nullptr);
            ret != FMOD_OK)
#else
        if (const auto ret =
                s_System->initialize(DefaultChannelCount, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
            ret != FMOD_OK)
#endif
        {
            cx_throw(AudioInitException, "Failed to initialize FMOD system. Err code: {}", EnumName(ret));
        }
    }

    void AudioSystem::Dispose() noexcept
    {
        if (s_System)
        {
            s_System->release();
            s_System = nullptr;
            lgx::Get("engine").Info("Audio subsystem disposed.");
        }
    }

    Studio::System* AudioSystem::GetFMODSystem() noexcept
    {
        return s_System;
    }

    bool AudioSystem::IsValid() noexcept
    {
        return s_System;
    }

    void AudioSystem::Update()
    {
        if (!IsValid())
        {
            cx_throw(AudioInitException, "Audio subsystem has not been initialized.");
        }

        if (const auto ret = s_System->update(); ret != FMOD_OK)
        {
            lgx::Get("engine").Error("FMOD update failed! Err code: {}", EnumName(ret));
        }
    }

    void AudioSystem::SetListenerAttributes(const SpatialAttributes& attr)
    {
        if (!IsValid())
        {
            cx_throw(AudioInitException, "Audio subsystem has not been initialized.");
        }

        FMOD_3D_ATTRIBUTES fattr;
        fattr.position = { attr.position.x, attr.position.y, attr.position.z };
        fattr.forward  = { attr.forward.x, attr.forward.y, attr.forward.z };
        fattr.up       = { attr.up.x, attr.up.y, attr.up.z };
        fattr.velocity = { attr.velocity.x, attr.velocity.y, attr.velocity.z };
        s_System->setListenerAttributes(0, &fattr);
    }
} // namespace codex::ax
