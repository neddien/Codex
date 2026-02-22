#include "VFS.h"

#include "MemoryMount.h"

namespace codex::fs {
    VFS::VFS() noexcept
    {
        m_Root              = mem::Box<Node>::New();
        m_Root->isDirectory = true;
        m_Root->owner       = nullptr;

        auto mem_mount = mem::Shared<MemoryMount>::New();
        m_Mounts.push_back(mem_mount);

        m_DefaultWriteMount = mem_mount.Get();

        Mount(mem_mount, "/");
    }

    void VFS::Mount(mem::Shared<IVFSMount> mount, const std::string_view path)
    {
    }

} // namespace codex::fs
