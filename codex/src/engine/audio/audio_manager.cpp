#include "audio_manager.h"

#include <engine/audio/audio_system.h>
#include <engine/concurrency/public/mutex.h>
#include <engine/core/common_internal.h>

#include <fmod_errors.h>
#include <fmod_studio.hpp>

namespace codex::ax {
    using namespace FMOD;

    static std::recursive_mutex                           s_mutex;
    static std::unordered_map<std::string, Studio::Bank*> s_bank_map{};

    std::future<EventHandle> AudioManager::load_event_async(const std::string_view event_path)
    {
        return std::async(
            std::launch::async,
            [event_path]
            {
                Studio::EventDescription* desc{};
                Studio::System*           system = AudioSystem::fmod_system();
                if (auto ret = system->getEvent(event_path.data(), &desc); ret != FMOD_OK) {
                    throw AudioException("Failed to find FMOD event '{}': Err code: {}", event_path,
                                         FMOD_ErrorString(ret));
                }

                desc->loadSampleData();

                FMOD_STUDIO_LOADING_STATE state;
                do {
                    // Drive FMOD's async processing so the load can complete,
                    // even if the main thread is blocked waiting on this future.
                    ax::AudioSystem::update();

                    if (const auto ret = desc->getSampleLoadingState(&state); ret != FMOD_OK) {
                        throw AudioException("Failed to load event sample data. Err code: {}", FMOD_ErrorString(ret));
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } while (state == FMOD_STUDIO_LOADING_STATE_LOADING);

                Studio::EventInstance* inst{};
                if (auto ret = desc->createInstance(&inst); ret != FMOD_OK) {
                    throw AudioException("Failed to create instance for '{}': {}", event_path, FMOD_ErrorString(ret));
                }

                return EventHandle{ inst };
            });
    }

    EventHandle AudioManager::load_event(const std::string_view event_path)
    { return load_event_async(event_path).get(); }

    std::future<void> AudioManager::load_bank_async(const std::filesystem::path& bank_path)
    {
        auto future = std::async(
            std::launch::async,
            [bank_path]()
            {
                std::scoped_lock guard{ s_mutex };

                if (!std::filesystem::exists(bank_path)) {
                    throw AudioException("Failed to load FMOD bank {}: No such file or directory.", bank_path.string());
                } else if (s_bank_map.contains(bank_path.string())) {
                    throw AudioException("Failed to load FMOD bank {}: Bank has already been loaded.",
                                         bank_path.string());
                }

                Studio::Bank*   bank{};
                Studio::System* system   = AudioSystem::fmod_system();
                auto            path_str = bank_path.string();

                if (const auto ret = system->loadBankFile(path_str.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
                    ret != FMOD_OK) {
                    throw AudioException("Failed to load FMOD bank {}: Err code: {}", path_str, FMOD_ErrorString(ret));
                }

                s_bank_map[std::move(path_str)] = bank;
            });

        return future;
    }

    void AudioManager::load_bank(const std::filesystem::path& bank_path)
    {
        // Cheap function, we can just await the async version of this method.
        // You will use the async version of this only for parallel loading.
        load_bank_async(bank_path).get();
    }

    void AudioManager::unload_bank(const std::filesystem::path& bank_path)
    {
        std::scoped_lock guard{ s_mutex };
        if (auto it = s_bank_map.find(bank_path.string()); it != s_bank_map.end()) {
            it->second->unload();
            s_bank_map.erase(it);
        }

        throw AudioException("Failed to unload bank {}: no such bank", bank_path.generic_string());
    }

    void AudioManager::unload_all()
    {
        std::scoped_lock guard{ s_mutex };
        Studio::System*  system = AudioSystem::fmod_system();
        system->unloadAll();
        s_bank_map.clear();
    }

    std::vector<std::string> AudioManager::get_all_event_paths()
    {
        std::scoped_lock         guard{ s_mutex };
        std::vector<std::string> paths;
        for (const auto& [_, bank] : s_bank_map) {
            int event_count = 0;
            bank->getEventCount(&event_count);
            if (event_count <= 0)
                continue;

            std::vector<Studio::EventDescription*> events(event_count);
            bank->getEventList(events.data(), event_count, &event_count);

            for (int i = 0; i < event_count; ++i) {
                int path_len = 0;
                events[i]->getPath(nullptr, 0, &path_len);

                std::string path(path_len - 1, '\0');
                events[i]->getPath(path.data(), path_len, &path_len);
                paths.push_back(std::move(path));
            }
        }
        std::sort(paths.begin(), paths.end());
        return paths;
    }

    std::vector<EventParameterInfo> AudioManager::event_parameters(const std::string_view event_path)
    {
        std::vector<EventParameterInfo> params;
        Studio::System*                 system = AudioSystem::fmod_system();

        Studio::EventDescription* desc{};
        if (system->getEvent(event_path.data(), &desc) != FMOD_OK)
            return params;

        int count = 0;
        desc->getParameterDescriptionCount(&count);

        for (int i = 0; i < count; ++i) {
            FMOD_STUDIO_PARAMETER_DESCRIPTION pdesc{};
            if (desc->getParameterDescriptionByIndex(i, &pdesc) != FMOD_OK)
                continue;

            // Skip built-in parameters (distance, direction, etc.)
            if (pdesc.flags & FMOD_STUDIO_PARAMETER_READONLY)
                continue;
            if (pdesc.type != FMOD_STUDIO_PARAMETER_GAME_CONTROLLED)
                continue;

            params.push_back({
                .name          = pdesc.name,
                .minimum       = pdesc.minimum,
                .maximum       = pdesc.maximum,
                .default_value = pdesc.defaultvalue,
            });
        }

        return params;
    }

    void AudioManager::set_master_volume(const f32 volume)
    {
        Studio::Bus* masterBus{};
        AudioSystem::fmod_system()->getBus("bus:/", &masterBus);
        masterBus->setVolume(volume);
    }

    void AudioManager::pause_all()
    {
        Studio::Bus* masterBus{};
        AudioSystem::fmod_system()->getBus("bus:/", &masterBus);
        masterBus->setPaused(true);
    }

    void AudioManager::resume_all()
    {
        Studio::Bus* masterBus{};
        AudioSystem::fmod_system()->getBus("bus:/", &masterBus);
        masterBus->setPaused(false);
    }

    void AudioManager::stop_all()
    {
        Studio::Bus* masterBus{};
        AudioSystem::fmod_system()->getBus("bus:/", &masterBus);
        masterBus->stopAllEvents(FMOD_STUDIO_STOP_ALLOWFADEOUT);
    }
} // namespace codex::ax
