#include "audio_system.h"

#include <engine/concurrency/public/mutex.h>
#include <engine/core/common_internal.h>
#include <engine/memory/public/memory.h>

#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_studio.hpp>

namespace codex::ax {
    using namespace FMOD;

    namespace detail {
        void mklog(const LogLevel level, const char* message)
        { AudioSystem::get().log(level, "FMod: {}", message); }
    } // namespace detail

#ifdef CX_BUILD_TYPE_DEBUG
    static FMOD_RESULT F_CALL fmod_debug_callback(FMOD_DEBUG_FLAGS flags, [[maybe_unused]] const char* file,
                                                  [[maybe_unused]] int line, [[maybe_unused]] const char* func,
                                                  const char* message)
    {
        LogLevel level = LogLevel::Info;
        if (flags & FMOD_DEBUG_LEVEL_ERROR)
            level = LogLevel::Error;
        else if (flags & FMOD_DEBUG_LEVEL_WARNING)
            level = LogLevel::Warn;

        if (std::strlen(message) > 1) {
            std::string str{ message };
            if (!str.empty() && static_cast<i64>(str.size()) - 1 >= 0 && str[str.size() - 1] == '\n')
                str[str.size() - 1] = 0;

            detail::mklog(level, str.c_str());
        }
        return FMOD_OK;
    }
#endif

    AudioSystem& AudioSystem::get() noexcept
    {
        static AudioSystem selfance;
        return selfance;
    }

    void AudioSystem::init()
    {
        auto& self = get();

        if (is_valid()) {
            throw AudioInitException("Audio sub-system has already been initialized.");
        }

#ifdef CX_BUILD_TYPE_DEBUG
        FMOD::Debug_Initialize(FMOD_DEBUG_LEVEL_LOG | FMOD_DEBUG_LEVEL_WARNING | FMOD_DEBUG_LEVEL_ERROR,
                               FMOD_DEBUG_MODE_CALLBACK, fmod_debug_callback, nullptr);
#endif

        if (const auto ret = Studio::System::create(&self.fmod_sys_); ret != FMOD_OK) {
            throw AudioInitException("Failed to create an FMOD system. Err code: {}", FMOD_ErrorString(ret));
        }

#ifdef CX_BUILD_TYPE_DEBUG
        if (const auto ret =
                self.fmod_sys_->initialize(DefaultChannelCount, FMOD_STUDIO_INIT_NORMAL | FMOD_STUDIO_INIT_LIVEUPDATE,
                                           FMOD_INIT_NORMAL | FMOD_INIT_PROFILE_ENABLE, nullptr);
            ret != FMOD_OK)
#else
        if (const auto ret =
                self.fmod_sys_->initialize(DefaultChannelCount, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
            ret != FMOD_OK)
#endif
        {
            throw AudioInitException("Failed to initialize FMOD system. Err code: {}", FMOD_ErrorString(ret));
        }
    }

    void AudioSystem::dispose() noexcept
    {
        auto& self = get();

        if (self.fmod_sys_) {
            self.fmod_sys_->release();
            self.fmod_sys_ = nullptr;
            self.log(Info, "Audio subsystem diposed");
        }
    }

    Studio::System* AudioSystem::get_fmod_system() noexcept
    { return get().fmod_sys_; }

    bool AudioSystem::is_valid() noexcept
    { return get().fmod_sys_; }

    void AudioSystem::update()
    {
        auto& self = get();

        if (!is_valid()) {
            throw AudioInitException("Audio subsystem has not been initialized.");
        }

        if (const auto ret = self.fmod_sys_->update(); ret != FMOD_OK) {
            self.error("FMOD update failed! Err code: {}", FMOD_ErrorString(ret));
        }
    }

    void AudioSystem::set_listener_attributes(const SpatialAttributes& attr)
    {
        auto& self = get();

        if (!is_valid()) {
            throw AudioInitException("Audio subsystem has not been initialized.");
        }

        FMOD_3D_ATTRIBUTES fattr;
        fattr.position = { attr.position.x, attr.position.y, attr.position.z };
        fattr.forward  = { attr.forward.x, attr.forward.y, attr.forward.z };
        fattr.up       = { attr.up.x, attr.up.y, attr.up.z };
        fattr.velocity = { attr.velocity.x, attr.velocity.y, attr.velocity.z };
        self.fmod_sys_->setListenerAttributes(0, &fattr);
    }
} // namespace codex::ax
