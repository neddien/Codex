#include "Public/NativeBehaviourManager.h"

#include <Engine/System/DynamicLibrary.h>

namespace codex {

    NBMan& NBMan::Get()
    {
        static NBMan instance;
        return instance;
    }

    void NBMan::Load(const std::filesystem::path path)
    {
        auto& instance = Get();
        if (instance.m_NBInstance)
        {
            cx_throw(ScriptException, "An NBMan instance has already been loaded.");
        }

        instance.m_NBInstance = mem::Box<sys::DLib>::New(std::move(path));

        lgx::Get("engine").Log(lgx::Info, "Script module loaded: {}", instance.m_NBInstance->GetPath().string());
    }

    void NBMan::Unload()
    {
        auto& instance = Get();
        if (instance.m_NBInstance)
        {
            const auto path = instance.m_NBInstance->GetPath().string();
            instance.m_Types.clear();
            instance.m_NBInstance.Reset();
            lgx::Get("engine").Log(lgx::Info, "Script module unloaded: {}", path);
        }
        else
        {
            cx_throw(ScriptException, "No NBMan instance loaded.");
        }
    }

    bool NBMan::InstanceLoaded() noexcept
    {
        return Get().m_NBInstance;
    }
} // namespace codex