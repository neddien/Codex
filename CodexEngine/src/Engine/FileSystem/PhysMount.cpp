#include "PhysMount.h"

namespace codex::fs {
    void PhysMount::Enumerate(FSEnumCallback callback)
    {
    }

    std::vector<u8> PhysMount::Read(const std::filesystem::path& mountPath)
    {
        return {};
    }

    i32 PhysMount::GetPriority() const
    {
        return m_Priority;
    }
} // namespace codex::fs
