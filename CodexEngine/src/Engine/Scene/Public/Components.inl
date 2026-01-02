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

        mem::Box<NativeBehaviour> bh(new T(std::forward<TArgs>(args)...));
        bh->OnInit();
        // FIXME: Serialize properly w new serializaiton system
        // bh->Serialize();
        /*
        const std::string& name = bh->m_SerializedData.begin().key();
        if (!m_Behaviours.contains(name))
            m_Behaviours[name] = std::move(bh);
        */

        return *((T*)bh.Get());
        // return *reinterpret_cast<T*>(m_Behaviours[name].Get());
    }
} // namespace codex
