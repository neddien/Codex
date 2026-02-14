#pragma once

#include "Components.h"

#include <Engine/NativeBehaviour/Public/NativeBehaviour.h>

namespace codex {
    template <typename T, typename... TArgs>
    T& NativeBehaviourComponent::New(TArgs&&... args)
        requires(std::is_base_of_v<NativeBehaviour, T>)
    {
        for (const auto& [k, v] : m_Behaviours)
        {
            if (typeid(v) == typeid(T))
                cx_throwd(DuplicateBehaviourException);
        }

        auto bh = mem::Box<NativeBehaviour>::New(std::forward<TArgs>(args)...);
        bh->OnInit();

        return *(static_cast<T*>(bh.Get()));
    }
} // namespace codex
