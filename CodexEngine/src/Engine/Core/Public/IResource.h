#pragma once

#include <sdafx.h>

#include <Engine/Memory/Public/Memory.h>
#include <Engine/Core/Public/Serializer.h>

namespace codex {
    class IResource : public ISerializable
    {
    protected:
        usize m_Id = 0;

    public:
        IResource()          = default;
        virtual ~IResource() = default;

    public:
        constexpr usize GetId() const noexcept { return m_Id; }
    };

    template <typename T>
        requires(std::is_base_of_v<IResource, T>)
    using ResRef = mem::Shared<T>;
} // namespace codex
