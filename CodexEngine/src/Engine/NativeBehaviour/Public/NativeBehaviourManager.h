#pragma once

#include <sdafx.h>

#include <Engine/Memory/Public/Memory.h>
#include <Engine/NativeBehaviour/Public/NativeBehaviour.h>

namespace codex {
    namespace sys {
        class DLib;
    }

    class CODEX_API NBMan
    {
    public:
        using FactoryFn = std::function<mem::Box<NativeBehaviour>()>;

    public:
        NBMan() = default;

    public:
        [[nodiscard]] static NBMan& Get();

    public:
        template <typename T>
        static void RegisterType(const std::string& typeName)
        {
            Get().m_Types[typeName] = []() -> mem::Box<NativeBehaviour> { return mem::Box<T>::New(); };
        }

    public:
        static mem::Box<NativeBehaviour> CreateInstance(const std::string& typeName)
        {
            auto& types = Get().m_Types;
            auto  it    = types.find(typeName);
            if (it != types.end())
            {
                return std::move(it->second());
            }
            return nullptr;
        }
        static bool IsTypeRegistered(const std::string& typeName)
        {
            return Get().m_Types.find(typeName) != Get().m_Types.end();
        }
        static std::vector<std::string> GetRegisteredTypes()
        {
            std::vector<std::string> types;
            for (const auto& pair : Get().m_Types)
            {
                types.push_back(pair.first);
            }
            return types;
        }
        static void Load(const std::filesystem::path path);
        static void Unload();
        [[nodiscard]] static bool InstanceLoaded() noexcept;

    private:
        std::unordered_map<std::string, FactoryFn> m_Types;
        mem::Box<sys::DLib>                        m_NBInstance;
    };
}; // namespace codex