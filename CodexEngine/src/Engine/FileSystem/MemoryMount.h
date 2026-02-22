#pragma once

#include <sdafx.h>

#include "IVFSMount.h"

namespace codex::fs {
    class MemoryMount : public IVFSMount
    {
    private:
        struct FileEntry
        {
            std::vector<u8> buffer;
            u64             lastModif;
            bool            readOnly;
        };

    public:
        MemoryMount(const i32 priority);

    public:
        bool                     Exists(const std::string& path) override;
        std::vector<std::string> ListFiles() override;
        mem::Shared<FileHandle>  Open(const std::string& path, const FileProperties props = {}) noexcept override;
        i32                      GetPriority() const override;

    private:
        std::unordered_map<std::string, FileEntry> m_Files;
        i32                                        m_Priority;
    };
} // namespace codex::fs
