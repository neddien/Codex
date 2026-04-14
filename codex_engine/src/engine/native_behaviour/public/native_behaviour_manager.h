#pragma once

#include <engine/memory/public/memory.h>
#include <engine/native_behaviour/public/native_behaviour.h>

namespace codex {
    class Scene;

    namespace sys {
        class DLib;
    }

    class CODEX_API NBMan
    {
    public:
        using FactoryFn = std::function<Box<NativeBehaviour>()>;

    public:
        NBMan() = default;
        ~NBMan() noexcept;
        NBMan(const NBMan&)                = delete;
        NBMan& operator=(const NBMan&)     = delete;
        NBMan(NBMan&&) noexcept            = default;
        NBMan& operator=(NBMan&&) noexcept = default;

    public:
        [[nodiscard]] static NBMan& get();

    public:
        template <typename T>
        static void register_type(const std::string& type_name)
        {
            get().types_[type_name] = []() -> Box<NativeBehaviour> { return Box<T>::make(); };
        }

    public:
        [[nodiscard]] static Box<NativeBehaviour> create_instance(const std::string& type_name)
        {
            auto& types = get().types_;
            auto  it    = types.find(type_name);
            if (it != types.end())
                return std::move(it->second());
            return nullptr;
        }
        [[nodiscard]] static bool is_type_registered(const std::string& type_name)
        {
            return get().types_.find(type_name) != get().types_.end();
        }
        [[nodiscard]] static std::vector<std::string> registered_types()
        {
            std::vector<std::string> types;
            for (const auto& pair : get().types_)
                types.push_back(pair.first);
            return types;
        }
        static void               load(const std::filesystem::path path, Scene& scene);
        static void               unload(const bool save_to_pending = true);
        [[nodiscard]] static bool instance_loaded() noexcept;

    private:
        std::unordered_map<std::string, FactoryFn> types_;
        Box<sys::DLib>                        nb_instance_;
        Scene*                                     scene_ = nullptr;
    };
} // namespace codex
