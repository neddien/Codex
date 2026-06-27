#pragma once

#include "public/filesystem.h"

#ifdef None
#undef None
#endif

namespace codex::fs {
    constexpr u8 PakVersion[3] = {
        1,
        0,
        0,
    };

    enum class PakFlags : u16
    {
        None               = 0,
        Compressed         = bit(0),
        CompressionTypeLZ4 = bit(1),
        Insecure           = bit(2),
    };
    CX_ENABLE_BITWISE_ENUM(PakFlags);

    struct PakEntry
    {
        u64      path_hash;
        char     path[128]; // KMaxPathLength
        u64      data_offset;
        u64      compressed_size;
        u64      uncompressed_size;
        u64      last_modified;
        PakFlags compression_type;
        PakFlags flags;
        u32      crc;
        u64      chunk_count;
    };

    struct PakHeader
    {
        char     magic[10] = "CXPAKFILE";
        u8       version[3];
        u8       content_version[3];
        char     pak_name[64];
        PakFlags flags;
        u64      entry_count;
        u64      data_offset;
        u64      data_size;
        u64      chunk_size;
        u64      last_modified;
        u64      reserved;
    };

    struct PakProperties
    {
        bool        enable_compression    = true;
        u64         compression_threshold = 1024; // skip compression for files smaller than this
        std::string name                  = "generic-pak-file";
        PakFlags    flags                 = PakFlags::None;
        u8          version[3]            = {};
        u64         chunk_size            = 4096;

        // Extensions whose contents are already compressed — LZ4 won't help and just wastes CPU.
        // Users can add or replace entries as needed.
        std::vector<std::string> precompressed_exts = {
            ".png",  ".jpg", ".jpeg", ".webp", ".ogg", ".mp3", ".flac",
            ".opus", ".pak", ".zip",  ".gz",   ".bz2", ".xz",  ".7z",
        };
    };
} // namespace codex::fs
