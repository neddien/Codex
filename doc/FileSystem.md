# Codex Virtual Filesystem (VFS)

## Overview

The VFS is a trie-based virtual filesystem that unifies multiple backing stores (disk, memory, PAK archives) behind a single path namespace. Multiple mounts can attach to the same path node; the highest-priority mount wins on lookup.

Header: `<engine/filesystem/public/filesystem.h>`

---

## Core Concepts

### Mounts

A mount is an `IVFSMount` implementation attached to a VFS path. Three built-in types:

| Type | Class | Description |
|------|-------|-------------|
| Memory | `MemoryMount` | In-process byte buffers, no disk I/O |
| Disk | `DiskMount` | Delegates to `std::filesystem` |
| PAK archive | `PakMount` | Read-only compressed/uncompressed archive |

Mounts at the same node are sorted by priority (highest first). `open`/`exists` try each in order, returning on first hit.

### Path Normalization

All paths are normalized on entry: leading/trailing slashes stripped, consecutive slashes collapsed. `/assets//sprites/` → `assets/sprites`.

---

## VFS API

```cpp
VFS vfs;

// Mount management
vfs.mount(mem::Shared<DiskMount>::make("/game/assets", 0), "/assets");
vfs.unmount("/assets");            // remove all mounts at this node
vfs.unmount("/assets", my_mount);  // remove a specific mount

// File I/O
auto fh = vfs.open("assets/sprites/player.png", { FileMode::Read });
vfs.exists("assets/sprites/player.png");
vfs.mkdir("assets/generated", /*recursive=*/true);

// Directory listing
auto entries = vfs.list("assets/sprites");
auto files   = vfs.list("assets", ListOptions::FilesOnly | ListOptions::Recursive);
auto dirs    = vfs.list("assets", ListOptions::DirsOnly);

vfs.is_directory("assets/sprites");

// Async variants (runs on worker pool, returns cc::task<T>)
co_await vfs.open_async("assets/sprites/player.png");
co_await vfs.exists_async("assets/sprites/player.png");
co_await vfs.list_async("assets");
```

### `ListOptions`

Bitmask defined in `ivfs_mount.h`:

| Flag | Effect |
|------|--------|
| `ListOptions::None` | Immediate children, files and dirs |
| `ListOptions::Recursive` | Full subtree |
| `ListOptions::FilesOnly` | Exclude directories from results |
| `ListOptions::DirsOnly` | Exclude files from results |

---

## FileHandle

All I/O goes through `mem::Shared<FileHandle>`. Key methods:

```cpp
fh->read(dest, len);                    // sequential read, advances cursor
fh->write(src, len);                    // sequential write, advances cursor
fh->read_at(dest, len, offset);         // positional read, cursor unchanged
fh->write_at(src, len, offset);         // positional write, cursor unchanged
fh->seek(offset);
fh->tell();
fh->size();
fh->path();

// Async variants
co_await fh->read_async(dest, len);
co_await fh->write_async(src, len);
```

Platform implementations use `pread`/`pwrite` (POSIX) and `OVERLAPPED` ReadFile/WriteFile (Windows) for `read_at`/`write_at`, making them fully thread-safe with no cursor movement.

`FileProperties` controls open mode:

```cpp
auto fh = vfs.open("path", { FileMode::Read });
auto fh = vfs.open("path", { FileMode::Write });
auto fh = vfs.open("path", { FileMode::Read | FileMode::Write });
```

---

## PAK Archives

PAK (`.cxpak`) is a custom binary archive format with optional per-entry LZ4 compression.

### Binary Layout

```
[PakHeader]
[PakEntry × entry_count]
--- data region ---
  per compressed entry:
    [u64 × chunk_count]        ← chunk offset table (absolute file offsets)
    per chunk:
      [u32 comp_size][bytes]   ← framed LZ4 independent block
  per uncompressed entry:
    [raw bytes]
```

### `PakHeader` fields

| Field | Type | Description |
|-------|------|-------------|
| `magic` | `char[10]` | `"CXPAKFILE"` — validated on load |
| `version` | `u8[3]` | Engine PAK format version |
| `content_version` | `u8[3]` | User-defined content version |
| `pak_name` | `char[64]` | Human-readable archive name |
| `flags` | `PakFlags` | Archive-wide flags |
| `entry_count` | `u64` | Number of entries |
| `data_offset` | `u64` | Byte offset to start of data region |
| `chunk_size` | `u64` | Decompressed chunk size (default 4096) |

### `PakEntry` fields

| Field | Type | Description |
|-------|------|-------------|
| `path_hash` | `u64` | djb2 hash of path (used for O(log N) lookup) |
| `path` | `char[64]` | Normalized relative path |
| `data_offset` | `u64` | Absolute offset into PAK file |
| `compressed_size` | `u64` | Total compressed bytes (0 if uncompressed) |
| `uncompressed_size` | `u64` | Original file size |
| `flags` | `PakFlags` | Per-entry flags |
| `crc` | `u32` | CRC32 of uncompressed content |
| `chunk_count` | `u64` | Number of LZ4 chunks (0 if uncompressed) |

### `PakFlags`

```cpp
enum class PakFlags : u16 {
    None               = bit(0),
    Compressed         = bit(1),   // entry uses LZ4 compression
    CompressionTypeLZ4 = bit(2),   // compression algorithm identifier
    Insecure           = bit(3),   // skip CRC integrity verification on mount
};
```

---

## Exporting a PAK

```cpp
PakProperties props;
props.name                 = "game-assets";
props.flags                = PakFlags::Compressed;
props.chunk_size           = 4096;
props.compression_threshold = 1024;  // skip compression for files < 1 KB
props.version              = { 1, 0, 0 };

// Customize which extensions are treated as already-compressed
// (LZ4 skipped for these regardless of the Compressed flag):
props.precompressed_exts.push_back(".wasm");

auto out = vfs.open("output/game.cxpak", { FileMode::Write });
vfs.export_to_pak(out, props);
```

`export_to_pak` performs three phases:

1. **Enumerate** (under VFS read-lock): DFS the trie, list files from all mounts at each node. Duplicate paths across mounts are deduplicated — highest-priority mount's file wins.
2. **Write index** (lock released): Write `PakHeader` + full `PakEntry` table as placeholder.
3. **Write data**: For each entry, compress with LZ4 independent blocks (if applicable), stream CRC32 over the original bytes, seek back and patch the entry's `data_offset`, `compressed_size`, `chunk_count`, and `crc`.

Extensions in `props.precompressed_exts` are matched case-insensitively and skipped for compression regardless of the `Compressed` flag.

---

## Integrity Verification

On `PakMount` construction, if the archive does **not** carry `PakFlags::Insecure`, every entry is read back and its CRC32 verified against the stored `entry.crc`. A mismatch (or LZ4 decompression failure) throws `InvalidOperationException` listing the corrupt entry count.

```cpp
// Mount throws if any entry is corrupt (and PAK is not Insecure):
auto pak = mem::Shared<PakMount>::make(handle, /*priority=*/10);

// Manual re-check at any time:
auto corrupted = pak->verify_integrity();
if (!corrupted.empty()) {
    for (const auto& path : corrupted)
        error("Corrupt entry: {}", path);
}
```

`verify_integrity()` returns an empty vector immediately if `PakFlags::Insecure` is set — no CRC computation is performed at all.

**Threat model**: CRC32 reliably detects accidental corruption (disk rot, bad downloads, partial writes). It does **not** protect against a deliberate attacker who can also recompute the CRC. For untrusted distribution channels, consider an HMAC over the entire archive.

The `Insecure` flag is useful for development PAKs and hot-reload workflows where verification overhead is unwanted.

---

## `PakMount` Lookup Performance

`PakMount` entries are sorted by `path_hash` at load time. All lookups (`open`, `exists`, `is_directory`) use `std::lower_bound` binary search:

- **Files** (`open`/`exists`): binary search by `path_hash` → string verify for collision resolution. O(log N), cache-friendly (hash is the first field of `PakEntry`).
- **Directories** (`is_directory`/`exists`): binary search on a sorted `vector<string>` of implied directory paths built at load time. O(log D) where D = number of unique directories.

This avoids `unordered_map` bucket/node pointer chasing. For realistic archive sizes the contiguous binary search is faster in practice.

---

## Reading from a PAK

`PakFileHandle` supports both sequential and positional reads transparently:

```cpp
auto fh = pak->open("assets/sprites/player.png", { FileMode::Read });

// Sequential
u8 buf[1024];
fh->read(buf, sizeof(buf));

// Positional (no cursor change, thread-safe)
fh->read_at(buf, sizeof(buf), /*offset=*/4096);
```

For compressed entries the chunk offset table is cached in the handle at construction. Each `read_at` maps the requested byte range to the minimum set of chunks, decompresses into a scratch buffer (pre-allocated to `LZ4_COMPRESSBOUND(chunk_size)` at construction), and copies out the requested slice.
