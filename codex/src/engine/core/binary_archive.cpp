#include "public/binary_archive.h"

#include <cstring>

namespace codex {
    BinaryArchiveBackend::BinaryArchiveBackend() noexcept
        : saving_{ true }
    {
    }

    BinaryArchiveBackend::BinaryArchiveBackend(std::span<const u8> data) noexcept
        : saving_{ false }
        , in_{ data.data() }
        , in_size_{ data.size() }
    {
    }

    void BinaryArchiveBackend::ensure(const usize bytes) const
    {
        if (!saving_ && cursor_ + bytes > in_size_)
            throw DeserializationException("Binary archive underflow: tried to read {} bytes past the buffer.", bytes);
    }

    template <typename T>
    void BinaryArchiveBackend::rw_trivial(T& value)
    {
        if (saving_) {
            const u8* bytes = reinterpret_cast<const u8*>(&value);
            out_.insert(out_.end(), bytes, bytes + sizeof(T));
        } else {
            ensure(sizeof(T));
            std::memcpy(&value, in_ + cursor_, sizeof(T));
            cursor_ += sizeof(T);
        }
    }

    void BinaryArchiveBackend::rw_string(std::string& value)
    {
        if (saving_) {
            u64 len = value.size();
            rw_trivial(len);
            const u8* bytes = reinterpret_cast<const u8*>(value.data());
            out_.insert(out_.end(), bytes, bytes + value.size());
        } else {
            u64 len = 0;
            rw_trivial(len);
            ensure(len);
            value.assign(reinterpret_cast<const char*>(in_ + cursor_), len);
            cursor_ += len;
        }
    }

    void BinaryArchiveBackend::rw_count(usize& count)
    {
        u64 wire = static_cast<u64>(count);
        rw_trivial(wire);
        if (!saving_)
            count = static_cast<usize>(wire);
    }

#define CX_BIN_TRIVIAL(TYPE)                                                                                           \
    void BinaryArchiveBackend::trivial(const std::string_view, TYPE& value)                                            \
    {                                                                                                                  \
        rw_trivial(value);                                                                                             \
    }

    CX_BIN_TRIVIAL(u8)
    CX_BIN_TRIVIAL(i8)
    CX_BIN_TRIVIAL(u16)
    CX_BIN_TRIVIAL(i16)
    CX_BIN_TRIVIAL(u32)
    CX_BIN_TRIVIAL(i32)
    CX_BIN_TRIVIAL(u64)
    CX_BIN_TRIVIAL(i64)
    CX_BIN_TRIVIAL(f32)
    CX_BIN_TRIVIAL(f64)
    CX_BIN_TRIVIAL(bool)

#undef CX_BIN_PRIM

    void BinaryArchiveBackend::trivial(const std::string_view, std::string& value)
    {
        rw_string(value);
    }

    void BinaryArchiveBackend::begin_array(const std::string_view, usize& count)
    {
        rw_count(count);
    }
    void BinaryArchiveBackend::begin_map(const std::string_view, usize& count)
    {
        rw_count(count);
    }
    void BinaryArchiveBackend::map_key(std::string& key)
    {
        rw_string(key);
    }

    bool BinaryArchiveBackend::optional(const std::string_view, const bool present_on_save)
    {
        u8 present = saving_ ? (present_on_save ? 1 : 0) : 0;
        rw_trivial(present);
        return saving_ ? present_on_save : (present != 0);
    }
} // namespace codex
