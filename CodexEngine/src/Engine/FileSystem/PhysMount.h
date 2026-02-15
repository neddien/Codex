#pragma once

#include "IVFSMount.h"

namespace codex::fs {
    class PhysMount : public IVFSMount
    {
    public:
        virtual ~PhysMount() = default;

    public:
        void            Enumerate(FSEnumCallback callback) override;
        std::vector<u8> Read(const std::filesystem::path& mountPath) override;
        i32             GetPriority() const override;

    private:
        std::filesystem::path m_Root;
        i32                   m_Priority;
    };
} // namespace codex::fs
