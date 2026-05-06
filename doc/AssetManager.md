# Asset Manager Design Document

Design notes and research for implementing an Asset Manager with Virtual File System for the Codex Engine.

## Table of Contents

1. [How Other Engines Do It](#how-other-engines-do-it)
2. [Virtual Filesystem Pattern](#virtual-filesystem-pattern)
3. [Recommended Architecture for Codex](#recommended-architecture-for-codex)
4. [Editor vs Runtime](#editor-vs-runtime)
5. [Core Components](#core-components)
6. [Pak File Format](#pak-file-format)
7. [Cooking Pipeline](#cooking-pipeline)
8. [Compression Strategy](#compression-strategy)
9. [Implementation Phases](#implementation-phases)
10. [Design Decisions](#design-decisions)
11. [Sources](#sources)

---

## How Other Engines Do It

### Unreal Engine Approach

**Editor Mode:**
- Assets stored as individual `.uasset` files on disk
- Direct filesystem access for fast iteration
- Asset metadata database for quick lookups

**Runtime (Shipped Game):**
- Assets go through a **cooking pipeline** that:
  - Converts to platform-specific formats
  - Compresses and optimizes
  - Strips unused data
  - Compiles Blueprints to native code
- Cooked assets packaged into **Pak files** (`.pak`)
- Virtual filesystem mounts pak files transparently
- Can split into **chunks** for DLC/streaming

**Key Concepts:**
- Pak files are cooked for a specific platform (Windows pak incompatible with Android)
- Content is cooked for platform target and shader graphics language
- Chunking system allows assets to be split into independently distributable pieces
- Cook methods: "Cook By The Book" (ahead of time) or "File Server" (on-demand)

### Unity Approach

**Editor Mode:**
- Assets in `Assets/` folder with `.meta` sidecar files
- Asset database with GUIDs for tracking
- Direct loading via `AssetDatabase` API

**Runtime:**
- **AssetBundles** - archive containers for grouped assets
- **Addressables** system (higher-level abstraction):
  - Load by address string, not path
  - Automatic dependency resolution
  - Can be local or remote (CDN)
  - Catalog system maps addresses to locations

**Key Concepts:**
- Addressables decouple arrangement, building, and loading of content
- Giving an asset an address allows loading regardless of project location
- AssetBundles express dependencies between one another
- Runtime loading is asynchronous for flexibility
- Play Mode Scripts allow different loading behaviors during development

### Godot Approach

- Lightweight, node-based architecture
- Resources loaded via `load()` or `preload()`
- PCK files for packaging (similar concept to pak files)
- Scene files (`.tscn`) are text-based for version control friendliness

---

## Virtual Filesystem Pattern

Codex implements a trie-based VFS. The design is inspired by Simon Coenen's article but differs in several ways.

```
VFS Architecture (implemented):
┌─────────────────────────────────────────┐
│         codex::fs::VFS                  │
│  Trie of VFSNode; path-indexed          │
├─────────────────────────────────────────┤
│  IVFSMount interface (ivfs_mount.h)     │
│  ├── DiskMount   (std::filesystem)      │
│  ├── MemoryMount (in-process buffers)   │
│  └── PakMount    (.cxpkz archives)      │
├─────────────────────────────────────────┤
│  Per-node mount list, sorted by         │
│  priority desc. First hit wins.         │
│  Async ops via cc::Task<T> coroutines   │
└─────────────────────────────────────────┘
```

**Implemented:**
- `IVFSMount`: `exists`, `open` (returns `mem::Shared<FileHandle>`), `priority`, `mkdir`, `list` (with `ListOptions` bitmask), `is_directory`
- Async wrappers on every operation via `cc::Task<T>` (C++20 coroutines)
- Same code path for editor (DiskMount) and runtime (PakMount) — just different mounts
- Priority system: mounts at the same node tried highest-priority first

See `doc/FileSystem.md` for the full VFS/PAK API reference.

---

## Recommended Architecture for Codex

### High-Level Overview

```
┌────────────────────────────────────────────────────────┐
│                    AssetManager  [PARTIAL]              │
│  - Asset<T> load<T>(FileHandle)                        │
│  - Asset<T> load<T>(FileHandle, ImportSettings)        │
│  - register_loader<TLoader>(extensions)                │
│  - get_loader_by_ext / get_loader_by_type              │
│  [ ] caching, unload, UUID-based load                  │
├────────────────────────────────────────────────────────┤
│                  Asset Registry  [PARTIAL]              │
│  - scan(vfs, path) — async, generates .cxmeta sidecars │
│  - UUID ↔ path maps (internal)                         │
│  [ ] public UUID resolution, dependency graph          │
├────────────────────────────────────────────────────────┤
│           Virtual File System  [IMPLEMENTED]            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │  DiskMount   │  │  PakMount    │  │ MemoryMount  │ │
│  │  (Editor)    │  │  (Runtime)   │  │  (Embedded)  │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
└────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

| Layer | Responsibility |
|-------|----------------|
| **AssetManager** | High-level API, caching, ref-counting, type-safe loading |
| **Asset Registry** | UUID mapping, dependency tracking, metadata storage |
| **Virtual File System** | Unified file access across mount points |
| **Mount Points** | Physical dirs, pak files, memory, network |

---

## Editor vs Runtime

| Aspect           | Editor                   | Runtime                  |
|------------------|--------------------------|--------------------------|
| **Mount Source** | Physical directories     | Pak archives             |
| **Asset Format** | Source files + `.cxmeta` | Cooked/optimized         |
| **Loading**      | Direct filesystem        | VFS abstraction          |
| **Hot-reload**   | Yes (file watcher)       | No                       |
| **Metadata**     | JSON sidecar files       | Embedded in pak header   |
| **Compression**  | None                     | Per-file compression     |
| **Platform**     | Development host         | Target platform specific |

---

## Core Components

### 1. Asset Types & Handles (Implemented)

```cpp
// Base interface — all asset types inherit from this
class IAsset {
public:
    [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;
};

// Register a type name on an IAsset subclass (used by loaders)
// CX_ASSET(TypeName) — injects ktype_name() and type_name() override
CX_ASSET(Texture2D)

// Logical path + stable UUID
struct AssetPath : ISerializable {
    std::string path;
    UUID        uuid;
};

// Loaded asset result
template<AssetType TAsset>
struct Asset {
    Shared<TAsset> asset;
    AssetPath      path;
};
```

### 2. Virtual File System Interface (Implemented)

> Full API reference: `doc/FileSystem.md`

```cpp
// Mount point interface — codex/filesystem/ivfs_mount.h
class IVFSMount {
public:
    virtual ~IVFSMount() = default;

    [[nodiscard]] virtual bool                    exists(const std::string& path) const noexcept = 0;
    [[nodiscard]] virtual mem::Shared<FileHandle> open(const std::string& path,
                                                       FileProperties props = {}) noexcept = 0;
    [[nodiscard]] virtual i32                     priority() const noexcept = 0;

    virtual bool mkdir(const std::string& rel_path) noexcept { return true; }
    [[nodiscard]] virtual std::vector<std::string> list(const std::string& rel_path,
                                                        ListOptions opts = ListOptions::None) const noexcept { return {}; }
    [[nodiscard]] virtual bool is_directory(const std::string& rel_path) const noexcept { return false; }

    // Async wrappers — override for native async (io_uring etc.)
    [[nodiscard]] cc::Task<mem::Shared<FileHandle>>  open_async(std::string path, FileProperties props = {}) noexcept;
    [[nodiscard]] cc::Task<bool>                     exists_async(std::string path) const noexcept;
};

// VFS — codex/filesystem/vfs.h
class VFS {
public:
    VFS(mem::Shared<IVFSMount> default_mount = nullptr) noexcept;

    // Mount management (attach to a virtual path node)
    bool mount(mem::Shared<IVFSMount> mount, const std::string& path, bool mkdir = false) noexcept;
    bool unmount(const std::string& path, mem::Shared<IVFSMount> mount = nullptr) noexcept;

    // Sync file ops
    [[nodiscard]] mem::Shared<FileHandle>  open(const std::string& path, FileProperties props = {}) noexcept;
    [[nodiscard]] bool                     exists(const std::string& path) const noexcept;
    bool                                   mkdir(const std::string& path, bool recursive = false) noexcept;
    [[nodiscard]] std::vector<std::string> list(const std::string& dir, ListOptions opts = ListOptions::None) const noexcept;
    [[nodiscard]] bool                     is_directory(const std::string& path) const noexcept;

    // Async variants (return cc::Task<T>)
    [[nodiscard]] cc::Task<mem::Shared<FileHandle>>  open_async(std::string path, FileProperties props = {}) noexcept;
    [[nodiscard]] cc::Task<bool>                     exists_async(std::string path) const noexcept;
    [[nodiscard]] cc::Task<std::vector<std::string>> list_async(std::string dir) const noexcept;

    // Export trie contents to a .cxpak archive
    bool export_to_pak(mem::Shared<FileHandle> out, PakProperties props = {}) noexcept;
};
```

### 3. Disk Mount Point — Editor (Implemented)

```cpp
// codex/filesystem/disk_mount.h
class DiskMount : public IVFSMount, public mem::SharedManagable<DiskMount> {
public:
    DiskMount(std::filesystem::path root, i32 priority);

    [[nodiscard]] bool                    exists(const std::string& path) const noexcept override;
    [[nodiscard]] mem::Shared<FileHandle> open(const std::string& path, FileProperties props = {}) noexcept override;
    [[nodiscard]] i32                     priority() const noexcept override;
    bool                                   mkdir(const std::string& rel_path) noexcept override;
    [[nodiscard]] std::vector<std::string> list(const std::string& rel_path, ListOptions opts = ListOptions::None) const noexcept override;
    [[nodiscard]] bool                     is_directory(const std::string& rel_path) const noexcept override;
};

// Usage
auto disk = mem::Shared<DiskMount>::make("/project/assets", /*priority=*/0);
vfs.mount(disk, "assets");
```

### 4. Asset Registry (Implemented — partial)

```cpp
// Metadata sidecar (written as <asset_dir>/.meta/<filename>.cxmeta)
struct AssetMetadata : ISerializable {
    AssetPath              path;            // logical path + UUID
    std::string            filename;        // basename
    std::string            type;            // "Texture2D", "Shader", etc.
    u32                    checksum;        // CRC32 of file content
    std::vector<AssetPath> dependencies;
    Box<ISerializable>     import_settings; // loader-specific, nullable
};

class AssetRegistry : public Loggable<"AssetRegistry"> {
public:
    // Async recursive scan: generates/refreshes .cxmeta sidecars under path/.meta/
    cc::Task<void> scan(Shared<fs::VirtualFilesystem> vfs, const std::string path);

    // Update internal maps when an asset is renamed/moved
    void move_asset(const std::string& old_path, const std::string& new_path);
};
```

**Example .cxmeta sidecar file (JSON):**
```json
{
    "path": "assets/textures/player.png",
    "uuid": "550e8400-e29b-41d4-a716-446655440000",
    "filename": "player.png",
    "type": "Texture2D",
    "checksum": 3141592653,
    "dependencies": [],
    "import_settings": {
        "filter_mode": "Nearest",
        "mipmap_mode": "None",
        "wrap_mode": "Clamp",
        "format": "RGBA8"
    }
}
```

**Usage:**
```cpp
// Scan an asset directory — spawns worker tasks, writes .cxmeta files
co_await AssetManager::registry().scan(vfs, "assets/textures");
```

### 5. Asset Manager (Implemented — partial)

```cpp
// Singleton via System<AssetManager>
class AssetManager : public System<AssetManager>, public Loggable<"AssetManager"> {
public:
    // Load by FileHandle — uses registered loader for TAsset
    template<AssetType TAsset>
    static Asset<TAsset> load(Shared<fs::FileHandle> fh) noexcept;

    // Load with explicit import settings
    template<AssetType TAsset, Serializable TParam>
    static Asset<TAsset> load(Shared<fs::FileHandle> fh, const TParam& param) noexcept;

    // Loader registration (called during init or plugin setup)
    template<AssetLoader TLoader>
    static void register_loader(std::initializer_list<std::string_view> extensions) noexcept;

    // Loader lookup
    static Shared<IAssetLoader> get_loader_by_ext(const std::string& extension) noexcept;
    static Shared<IAssetLoader> get_loader_by_type(const std::string& type_name) noexcept;

    static AssetRegistry& registry() noexcept;

    void init();    // Registers built-in loaders (Texture2D for png/jpg/jpeg/bmp/gif)
    void dispose();
};

// Loader base — inherit from this to implement a loader
// TAssetImportSettings = void for loaders without settings
template<AssetType TAsset, ImportSettingsType TAssetImportSettings>
class AssetLoaderBase : public IAssetLoader {
public:
    virtual Shared<TAsset> load(Shared<fs::FileHandle> fh,
                                const TAssetImportSettings& params) const noexcept = 0;
};

// Example: Texture2D loader (stub — load() returns empty for now)
class Texture2DLoader : public AssetLoaderBase<Texture2D, Texture2D::ImportSettings> {
    Shared<Texture2D> load(Shared<fs::FileHandle> fh,
                           const Texture2D::ImportSettings& params) const noexcept override;
};
```

**Usage:**
```cpp
// Open a file via VFS, then load
auto fh  = vfs.open("assets/textures/player.png", { FileMode::Read });
auto tex = AssetManager::load<gfx::Texture2D>(fh);
// tex.asset — Shared<Texture2D>
// tex.path  — AssetPath with UUID
```

---

## Pak File Format (Implemented)

See `doc/FileSystem.md` for the full binary layout reference. Summary:

### Binary Layout

```
┌─────────────────────────────────────────┐
│ PakHeader (fixed size)                  │
│  - magic[10]:        "CXPAKFILE"        │
│  - version[3]:       engine format ver  │
│  - content_version[3]: user-defined     │
│  - pak_name[64]:     human-readable     │
│  - flags:            PakFlags (u16)     │
│  - entry_count:      u64                │
│  - data_offset:      u64                │
│  - data_size:        u64                │
│  - chunk_size:       u64 (default 4096) │
├─────────────────────────────────────────┤
│ PakEntry × entry_count                  │
│  - path_hash:        u64 (djb2)         │
│  - path[64]:         normalized path    │
│  - data_offset:      u64                │
│  - compressed_size:  u64                │
│  - uncompressed_size: u64               │
│  - flags:            PakFlags (u16)     │
│  - crc:              u32 (CRC32)        │
│  - chunk_count:      u64                │
├─────────────────────────────────────────┤
│ Data region                             │
│  Per compressed entry:                  │
│    [u64 × chunk_count] chunk offsets    │
│    per chunk: [u32 comp_size][bytes]    │
│  Per uncompressed entry:                │
│    [raw bytes]                          │
└─────────────────────────────────────────┘
```

### Pak Mount Implementation (Implemented)

```cpp
// codex/filesystem/pak_mount.h
class PakMount : public IVFSMount, public mem::SharedManagable<PakMount> {
public:
    // Takes an open FileHandle (use DiskMount or MemoryMount to open the .cxpak file)
    PakMount(mem::Shared<FileHandle> handle, i32 priority);

    [[nodiscard]] bool                     exists(const std::string& path) const noexcept override;
    [[nodiscard]] mem::Shared<FileHandle>  open(const std::string& path, FileProperties props = {}) noexcept override;
    [[nodiscard]] i32                      priority() const noexcept override;
    [[nodiscard]] std::vector<std::string> list(const std::string& rel_path, ListOptions opts = ListOptions::None) const noexcept override;
    [[nodiscard]] bool                     is_directory(const std::string& rel_path) const noexcept override;
    [[nodiscard]] u64                      chunk_size() const noexcept;

    // Recomputes CRC32 for every entry; returns paths of any corrupt entries.
    // Returns immediately (empty) if PakFlags::Insecure is set.
    [[nodiscard]] std::vector<std::string> verify_integrity() const;
};

// Usage: open a .cxpak file through the VFS, then mount it
auto pak_fh  = vfs.open("game.cxpak", { FileMode::Read });
auto pak_mnt = mem::Shared<PakMount>::make(pak_fh, /*priority=*/10);
vfs.mount(pak_mnt, "assets");
```

**Internals:**
- Entries sorted by `path_hash` (djb2) at load time → O(log N) binary search for all lookups
- Per-entry LZ4 independent block compression, chunk-based for random-access reads
- `PakEntry::crc` (CRC32) verified on mount unless `PakFlags::Insecure` is set
- See `doc/FileSystem.md` for the full `.cxpak` binary format

---

## Cooking Pipeline

### Overview

```
Source Assets (.png, .wav, .cxscene, .cxprefab)
         │
         ▼
    ┌─────────────┐
    │  Importer   │  Read source format, validate
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │  Converter  │  Platform-specific conversion
    └──────┬──────┘  (texture formats, endianness, etc.)
           │
           ▼
    ┌─────────────┐
    │  Optimizer  │  Compress textures, generate mipmaps,
    └──────┬──────┘  strip debug info, etc.
           │
           ▼
    ┌─────────────┐
    │   Packer    │  Bundle into .pak files
    └──────┬──────┘
           │
           ▼
    game.pak (or chunked: base.pak, level1.pak, dlc.pak)
```

### Cook Tool Interface

```cpp
// Asset cooker interface
class IAssetCooker {
public:
    virtual ~IAssetCooker() = default;

    // Cook a single asset
    virtual std::vector<u8> cook(const std::vector<u8>& sourceData,
                                  const AssetMetadata& metadata,
                                  const CookSettings& settings) = 0;

    virtual std::string type_name() const = 0;
    virtual std::vector<std::string> extensions() const = 0;
};

// Cook settings
struct CookSettings {
    std::string target_platform;  // "windows", "linux", "osx"
    bool enable_compression = true;
    u8 compression_type = 1;  // LZ4
    i32 compression_level = 6;
    bool strip_debug_info = true;
    bool generate_mipmaps = true;
    i32 max_texture_size = 4096;
};
```

---

## Compression Strategy

### Best Practices

| Aspect | Recommendation |
|--------|----------------|
| **Granularity** | Per-file, not whole archive (enables random access) |
| **Algorithm** | LZ4 for speed, ZSTD for better ratio |
| **Threshold** | Skip files < 0.5MB (compression overhead not worth it) |
| **Pre-compressed** | Skip already compressed formats (.png, .jpg, .ogg) |
| **Security** | Encrypt headers only, not data (faster) |

### Compression Types (Implemented)

```cpp
// PakFlags encode compression per-entry (codex/filesystem/cxpak.h)
enum class PakFlags : u16 {
    None               = 0,
    Compressed         = bit(0),    // entry uses LZ4 compression
    CompressionTypeLZ4 = bit(1),    // algorithm identifier (LZ4 independent blocks)
    Insecure           = bit(2),    // skip CRC verification on mount
};
```

Only LZ4 is implemented. ZSTD support is a future option. Extensions in `PakProperties::precompressed_exts` (`.png`, `.jpg`, `.ogg`, etc.) are skipped for compression regardless of the `Compressed` flag.

### Security Considerations

From Simon Coenen's article:
- Custom binary format provides basic obscurity
- Data blobs are meaningless without headers
- **Encrypt TOC/headers only** - much faster than encrypting all data
- Optional: per-file encryption for sensitive assets

---

## Implementation Phases

### Phase 1: Foundation — **COMPLETE**
- ~~Create `VirtualFileSystem` class with mount point interface~~ → `codex::fs::VFS` (trie-based)
- ~~Implement `PhysicalMount` for directory access~~ → `DiskMount`
- `MemoryMount` for in-process buffers
- Platform `FileHandle` implementations: `LinuxFileHandle` (`pread`/`pwrite`), `NtFileHandle` (Windows `OVERLAPPED`)
- Async file ops via `cc::Task<T>` (C++20 coroutines)

### Phase 2: Asset Registry (Editor) — **PARTIAL**
- [x] `.cxmeta` sidecar file format (JSON, `AssetMetadata` serializable)
- [x] `AssetRegistry` with internal UUID ↔ path maps
- [x] `AssetRegistry::scan(vfs, path)` — async directory walk, CRC32 checksumming, `.cxmeta` generation
- [ ] Public UUID resolution API (`uuid(path)`, `path(uuid)`)
- [ ] Dependency tracking between assets

### Phase 3: Asset Manager — **PARTIAL**
- [x] `AssetManager` singleton (`System<AssetManager>`)
- [x] `load<TAsset>(fh)` and `load<TAsset>(fh, params)` typed load API
- [x] `register_loader<TLoader>(extensions)` with lookup by extension and type name
- [x] `Texture2DLoader` registered for `png/jpg/jpeg/bmp/gif`; `Texture2D::ImportSettings` serializable
- [x] `init()` / `dispose()` lifecycle
- [ ] Asset caching / ref-counting
- [ ] `unload()` / `unload_unused()`
- [ ] `Texture2DLoader::load()` body (currently returns empty `Shared<Texture2D>`)
- [ ] Loaders for Shader, Prefab, SpriteSheet, Animation

### Phase 4: Editor Integration — **PLANNED**
1. Implement `ContentBrowserView` using VFS and Registry
2. Asset import pipeline with per-type settings
3. File watcher for hot-reload detection
4. Drag-and-drop asset references

### Phase 5: Runtime Packaging — **COMPLETE**
- ~~Define pak file binary format~~ → `.cxpak` (magic `"CXPAKFILE"`, LZ4 chunked compression)
- ~~Implement `PakMount` for reading pak files~~ → `PakMount` (binary search on `path_hash`, CRC32 integrity)
- ~~Create `PakBuilder`~~ → `VFS::export_to_pak(FileHandle, PakProperties)` writes `.cxpak` directly
- `PakProperties` controls name, compression threshold, chunk size, and pre-compressed extension list

### Phase 6: Cooking Pipeline — **PLANNED**
1. Asset cooker interface and registry
2. Platform-specific texture cooking
3. ZSTD compression option alongside LZ4
4. Command-line cook tool

---

## Design Decisions

### 1. UUID vs Path References

| Approach | Pros | Cons |
|----------|------|------|
| **Path-based** | Simple, human-readable | Breaks on rename/move |
| **UUID-based** | Survives renames, stable | Need registry, less obvious |
| **Hybrid** | Best of both | More complexity |

**Recommendation:** Hybrid - store UUID in `.cxmeta`, use paths in editor UI, resolve to UUID for runtime references.

### 2. Metadata Storage

| Approach | Pros | Cons |
|----------|------|------|
| **Sidecar files** | VCS-friendly, easy to edit | Extra files |
| **Embedded in asset** | Single file per asset | Format changes needed |
| **Central database** | Fast queries | Merge conflicts |

**Recommendation:** Sidecar `.cxmeta` files for editor, embedded in pak for runtime.

### 3. Compression Algorithm

| Algorithm | Compression Speed | Decompression Speed | Ratio |
|-----------|-------------------|---------------------|-------|
| **LZ4** | Very fast | Very fast | Moderate |
| **ZSTD** | Fast | Fast | Good |
| **LZ4HC** | Slow | Very fast | Better than LZ4 |

**Recommendation:** LZ4 as default (runtime speed priority), ZSTD optional for shipping builds.

### 4. Pak File Strategy

| Approach | Use Case |
|----------|----------|
| **Single pak** | Small games, simple deployment |
| **Chunked** | DLC, streaming, partial updates |
| **Per-level** | Large games, memory management |

**Recommendation:** Start with single pak, add chunking support later when needed.

---

## Sources

### Unreal Engine
- [Asset Management in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine)
- [Packaging Your Project](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project)
- [Build Operations: Cooking, Packaging, Deploying](https://dev.epicgames.com/documentation/en-us/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine)
- [Cooking Content and Creating Chunks](https://dev.epicgames.com/documentation/en-us/unreal-engine/cooking-content-and-creating-chunks-in-unreal-engine)
- [Loading Pak Files at Runtime](https://dev.epicgames.com/community/learning/knowledge-base/D7nL/unreal-engine-primer-loading-content-and-pak-files-at-runtime)

### Unity
- [Simplify Content Management with Addressables](https://unity.com/how-to/simplify-your-content-management-addressables)
- [Addressables Development Cycle](https://docs.unity3d.com/Packages/com.unity.addressables@1.3/manual/AddressableAssetsDevelopmentCycle.html)
- [Addressables Planning and Best Practices](https://blog.unity.com/engine-platform/addressables-planning-and-best-practices)
- [Asset Dependencies](https://docs.unity3d.com/Packages/com.unity.addressables@2.7/manual/AssetDependencies.html)
- [Introduction to Runtime Asset Management](https://docs.unity3d.com/6000.3/Documentation/Manual/assets-managing-introduction.html)

### Technical References
- [Pak Files - Virtual File System (Simon Coenen)](https://simoncoenen.com/blog/programming/PakFiles) - Excellent deep-dive on VFS and pak file implementation
- [A Proper Guide to Game Asset Management](https://www.anchorpoint.app/blog/a-proper-guide-to-game-asset-management)

### Compression Libraries
- [LZ4](https://github.com/lz4/lz4) - Extremely fast compression
- [ZSTD (Zstandard)](https://github.com/facebook/zstd) - Fast compression with good ratios
- [Oodle](http://www.radgametools.com/oodle.htm) - Industry-standard (proprietary, used by many AAA games)
