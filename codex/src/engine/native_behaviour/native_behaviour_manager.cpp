#include "public/native_behaviour_manager.h"

#include <engine/scene/public/components.h>
#include <engine/scene/public/entity.inl>
#include <engine/scene/public/scene.h>
#include <engine/system/dynamic_library.h>

namespace codex {
    NBMan::NBMan() noexcept  = default;
    NBMan::~NBMan() noexcept = default;

    void NBMan::init()
    {
        auto& self = get();
        self.log(Info, "Subsystem initialized");
    }

    void NBMan::dispose() noexcept
    {
        auto& self = get();
        if (self.nb_instance_) {
            const auto path = self.nb_instance_->get_path().string();
            self.types_.clear();
            self.nb_instance_.reset();
            self.scene_ = nullptr;
        }

        self.log(Info, "Subsystem disposed");
    }

    void NBMan::load(const std::filesystem::path path, Scene& scene)
    {
        auto& self = get();
        if (self.nb_instance_) {
            throw ScriptException("An NBMan instance has already been loaded.");
        }

        self.scene_       = &scene;
        self.nb_instance_ = Box<sys::DLib>::make(std::move(path));

        self.log(Info, "Script module loaded: {}", self.nb_instance_->get_path().string());
    }

    void NBMan::unload(const bool save_to_pending)
    {
        auto& self = get();
        if (self.nb_instance_) {
            // Live behaviours hold vtables pointing into the module being unloaded, so
            // they must be destroyed before the module is, whether or not their names
            // are saved for re-attachment.
            if (self.scene_) {
                auto nbc_view = self.scene_->entities_with_component<NativeBehaviourComponent>();
                for (auto& e : nbc_view) {
                    auto& nbc = e.get_component<NativeBehaviourComponent>();
                    if (save_to_pending)
                        nbc.dispose_and_save_attached_to_pending();
                    else
                        nbc.dispose_behaviours();
                }
            }

            const auto path = self.nb_instance_->get_path().string();
            self.types_.clear();
            self.nb_instance_.reset();
            self.scene_ = nullptr;
            self.log(Info, "Script module unloaded: {}", path);
        } else {
            throw ScriptException("No NBMan instance loaded.");
        }
    }

    bool NBMan::is_type_registered(const std::string_view type_name)
    {
        /* clang-format: no inline */
        auto& self = get();
        return self.types_.contains(util::crypto::fnv1a(type_name));
    }

    std::vector<std::string> NBMan::registered_types() noexcept
    {
        auto&                    self = get();
        std::vector<std::string> types;
        types.reserve(self.types_.size());
        for (const auto& [x, y] : self.types_)
            types.push_back(y.type_name);
        return types;
    }

    bool NBMan::instance_loaded() noexcept
    { return get().nb_instance_; }

    const NBMan::BHRecord* NBMan::type_record(const std::string_view type) noexcept
    {
        auto&       self = get();
        const usize hash = util::crypto::fnv1a(type);
        return (self.types_.contains(hash)) ? &self.types_[hash] : nullptr;
    }
} // namespace codex
