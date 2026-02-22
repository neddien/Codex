#include "MemoryMount.h"

#include "FileHandle.h"

namespace codex::fs {
    class MemoryFileHandle : public FileHandle
    {
    public:
        MemoryFileHandle(std::vector<u8>& data, const FileProperties props = FileProperties{})
            : m_Props{ props }
            , m_Data{ data }
            , m_Cursor{ 0 }
        {
        }

    public:
        usize Read(void* buffer, const usize len) override
        {
            if ((u8)m_Props.mode & (u8)FileMode::Read)
            {
                const usize avail    = m_Data.size() - m_Cursor;
                const usize read_len = std::min(avail, len);

                std::memcpy(buffer, m_Data.data() + m_Cursor, len);
                m_Cursor += read_len;

                return read_len;
            }

            return 0;
        }
        usize Write(void* buffer, const usize len) override
        {
            if ((u8)m_Props.mode & ((u8)FileMode::Write | (u8)FileMode::Append))
            {
                const usize total_len = m_Cursor + len;
                if (m_Data.size() <= total_len)
                {
                    m_Data.resize(total_len);
                }

                std::memcpy(m_Data.data() + m_Cursor, buffer, len);
                m_Cursor += len;

                return len;
            }

            return 0;
        }
        void  Seek(const usize offset) override { m_Cursor = offset; }
        usize Tell() override { return m_Cursor; }
        usize Size() override { return m_Data.size(); }

    public:
        FileHandle& operator<<(const std::vector<u8>& src) override
        {
            if ((u8)m_Props.mode & ((u8)FileMode::Write | (u8)FileMode::Append))
            {
                const usize total_len = m_Cursor + src.size();
                if (m_Data.size() <= total_len)
                {
                    m_Data.resize(total_len);
                }

                std::memcpy(m_Data.data() + m_Cursor, src.data(), src.size());
                m_Cursor += src.size();
            }

            return *this;
        }
        FileHandle& operator>>(std::vector<u8>& dest) override
        {
            if ((u8)m_Props.mode & (u8)FileMode::Read)
            {
                const usize read_len = m_Data.size() - m_Cursor;

                dest.resize(read_len);

                std::memcpy(dest.data(), m_Data.data() + m_Cursor, read_len);
                m_Cursor += read_len;
            }

            return *this;
        }

    private:
        std::vector<u8>& m_Data;
        usize            m_Cursor;
        FileProperties   m_Props;
    };

    MemoryMount::MemoryMount(const i32 priority)
        : m_Priority{ priority }
    {
    }

    bool MemoryMount::Exists(const std::string& path)
    {
    }

    std::vector<std::string> MemoryMount::ListFiles()
    {
    }

    mem::Shared<FileHandle> MemoryMount::Open(const std::string& path, const FileProperties props) noexcept
    {
        auto file_it = m_Files.find(path);
        if (file_it != m_Files.end())
        {
            FileEntry& entry = file_it->second;
            auto       mfh   = mem::Shared<MemoryFileHandle>::New(entry.buffer, props);
            return std::move(mfh);
        }

        if ((u8)props.mode & (u8)FileMode::Read)
        {
            return nullptr;
        }
        else if ((u8)props.mode & ((u8)FileMode::ReadWrite | (u8)FileMode::Write | (u8)FileMode::Append))
        {
            FileEntry entry{};
            auto      mfh = mem::Shared<MemoryFileHandle>::New(entry.buffer, props.mode);
            return std::move(mfh);
        }

        return nullptr;
    }

    i32 MemoryMount::GetPriority() const
    {
    }
} // namespace codex::fs
