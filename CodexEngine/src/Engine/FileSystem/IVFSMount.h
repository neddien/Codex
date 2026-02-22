#pragma once

#include <sdafx.h>

#include "FileHandle.h"

#include <Engine/Memory/Public/Memory.h>

namespace codex::fs {
    class IVFSMount
    {
    public:
        virtual ~IVFSMount()                                             = default;
        virtual bool                     Exists(const std::string& path) = 0;
        virtual std::vector<std::string> ListFiles()                     = 0;
        virtual mem::Shared<FileHandle>  Open(const std::string& path, const FileProperties props = {}) noexcept = 0;
        virtual i32                      GetPriority() const                                                     = 0;
    };
} // namespace codex::fs
