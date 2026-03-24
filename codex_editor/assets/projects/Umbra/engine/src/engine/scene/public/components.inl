#pragma once

#include "components.h"

#include <engine/native_behaviour/public/native_behaviour.h>

namespace codex {
    template <typename T, typename... TArgs>
    T& NativeBehaviourComponent::make_behaviour(TArgs&&... args)
        requires(std::is_base_of_v<NativeBehaviour, T>)
    {
        for (const auto& [k, v] : behaviours_) {
            if (typeid(v) == typeid(T))
                throw DuplicateBehaviourException();
        }

        auto bh = mem::Box<NativeBehaviour>::make(std::forward<TArgs>(args)...);
        bh->on_init();

        return *(static_cast<T*>(bh.get()));
    }
} // namespace codex
