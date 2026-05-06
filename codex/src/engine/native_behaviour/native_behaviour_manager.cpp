#include "public/native_behaviour_manager.h"

#include <engine/scene/public/components.h>
#include <engine/scene/public/scene.h>
#include <engine/system/dynamic_library.h>

namespace codex {
    NBMan::~NBMan() noexcept
    {
        if (nb_instance_) {
            const auto path = nb_instance_->get_path().string();
            types_.clear();
            nb_instance_.reset();
            scene_ = nullptr;
            info("~NBMan(): {}", path);
        }
    }

    NBMan& NBMan::get()
    {
        static NBMan instance;
        return instance;
    }

    void NBMan::load(const std::filesystem::path path, Scene& scene)
    {
        auto& instance = get();
        if (instance.nb_instance_) {
            throw ScriptException("An NBMan instance has already been loaded.");
        }

        instance.scene_       = &scene;
        instance.nb_instance_ = Box<sys::DLib>::make(std::move(path));

        instance.info("Script module loaded: {}", instance.nb_instance_->get_path().string());
    }

    void NBMan::unload(const bool save_to_pending)
    {
        auto& instance = get();
        if (instance.nb_instance_) {
            if (save_to_pending && instance.scene_) {
                auto nbc_view = instance.scene_->get_all_entities_with_component<NativeBehaviourComponent>();
                for (auto& e : nbc_view)
                    e.get_component<NativeBehaviourComponent>().save_attached_to_pending();
            }

            const auto path = instance.nb_instance_->get_path().string();
            instance.types_.clear();
            instance.nb_instance_.reset();
            instance.scene_ = nullptr;
            instance.info("Script module unloaded: {}", path);
        } else {
            throw ScriptException("No NBMan instance loaded.");
        }
    }

    bool NBMan::instance_loaded() noexcept
    {
        return get().nb_instance_;
    }
} // namespace codex
