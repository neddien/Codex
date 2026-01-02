#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/Serializer.h>
#include <Engine/Memory/Public/Memory.h>

namespace codex {
    class IResource : public ISerializable
    {
    protected:
        usize                 m_Id = 0;
        std::filesystem::path m_Path;

    public:
        IResource()          = default;
        virtual ~IResource() = default;

    public:
        usize                 GetId() const noexcept { return m_Id; }
        std::filesystem::path GetPath() const noexcept { return m_Path; }
    };

    template <typename T>
        requires(std::is_base_of_v<IResource, T>)
    using ResRef = mem::Shared<T>;
} // namespace codex
