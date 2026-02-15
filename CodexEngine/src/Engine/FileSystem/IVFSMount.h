#pragma once

#include <sdafx.h>

namespace codex::fs {
    using FSEnumCallback = std::function<void(const std::filesystem::path&)>;

    class IVFSMount
    {
    public:
        virtual ~IVFSMount()                                                 = default;
        virtual void            Enumerate(FSEnumCallback callback)           = 0;
        virtual std::vector<u8> Read(const std::filesystem::path& mountPath) = 0;
        virtual i32             GetPriority() const                          = 0;
    };
} // namespace codex::fs
