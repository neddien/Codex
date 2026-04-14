#pragma once

#include <engine/core/public/serializer.h>
#include <engine/memory/public/memory.h>

namespace codex {
    class IResource : public ISerializable
    {
    public:
        IResource()          = default;
        virtual ~IResource() = default;

    public:
        usize                 id() const noexcept { return id_; }
        std::filesystem::path path() const noexcept { return path_; }

    protected:
        usize                 id_ = 0;
        std::filesystem::path path_;
    };

    template <typename T>
        requires(std::is_base_of_v<IResource, T>)
    using ResRef = Shared<T>;
} // namespace codex
