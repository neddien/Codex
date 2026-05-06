#include "public/util.h"

#include <lz4.h>

namespace codex::util::io {
    std::vector<u8> compress_lz4(const void* src, const usize src_size) noexcept
    {
        const i32       capacity = LZ4_compressBound(static_cast<i32>(src_size));
        std::vector<u8> dst(capacity);
        const i32 written = LZ4_compress_default(static_cast<const char*>(src), reinterpret_cast<char*>(dst.data()),
                                                 static_cast<i32>(src_size), capacity);
        if (written <= 0)
            return {};

        dst.resize(static_cast<usize>(written));
        return dst;
    }

    bool decompress_lz4(const void* src, const usize compressed_size, void* dst, const usize original_size) noexcept
    {
        const i32 r = LZ4_decompress_safe(static_cast<const char*>(src), static_cast<char*>(dst),
                                          static_cast<i32>(compressed_size), static_cast<i32>(original_size));
        return r == static_cast<i32>(original_size);
    }
} // namespace codex::util::io
