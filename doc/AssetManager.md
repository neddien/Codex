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
│                    AssetManager  [PLANNED]              │
│  - AssetHandle<T> load(AssetPath)                      │
│  - cc::Task<AssetHandle<T>> load_async(AssetPath)      │
│  - unload(AssetHandle)                                 │
│  - get_metadata(AssetPath)                             │
├────────────────────────────────────────────────────────┤
│                  Asset Registry  [PLANNED]              │
│  - UUID → AssetPath mapping                            │
│  - Dependency graph                                    │
│  - Metadata cache (type, size, dependencies)           │
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

| Aspect | Editor | Runtime |
|--------|--------|---------|
| **Mount Source** | Physical directories | Pak archives |
| **Asset Format** | Source files + `.cxmeta` | Cooked/optimized |
| **Loading** | Direct filesystem | VFS abstraction |
| **Hot-reload** | Yes (file watcher) | No |
| **Metadata** | JSON sidecar files | Embedded in pak header |
| **Compression** | None | Per-file compression |
| **Platform** | Development host | Target platform specific |

---

## Core Components

### 1. Asset Path & Handle

```cpp
// Logical path, not filesystem path
struct AssetPath {
    std::string path;  // e.g., "textures/player/idle.png"
    UUID uuid;         // Stable across renames

    // Comparison operators for use in maps
    bool operator==(const AssetPath& other) const;
    bool operator<(const AssetPath& other) const;
};

// Type-safe handle with ref-counting
template<typename T>
class AssetHandle {

public:
    AssetHandle() = default;
    AssetHandle(const AssetPath& path);

    // Access operators
    T* operator->() { ensure_loaded(); return asset_.get(); }
    T& operator*() { EnsureLoaded(); return *asset_; }

    // State queries
    bool is_loaded() const { return loaded_; }
    bool is_valid() const { return !path_.path.empty(); }
    const AssetPath& path() const { return path_; }

private:
    void ensure_loaded();

private:
    AssetPath path_;
    Shared<T> asset_;  // Lazy-loaded
    bool loaded_ = false;
};
```

### 2. Virtual File System Interface (Implemented)

> Full API reference: `doc/FileSystem.md`

```cpp
// Mount point interface — codex/file_system/ivfs_mount.h
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

// VFS — codex/file_system/vfs.h
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
// codex/file_system/disk_mount.h
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

### 4. Asset Registry (Editor)

```cpp
// Metadata sidecar file structure (.cxmeta)
struct AssetMetadata {
    UUID uuid;
    std::string type;            // "Texture2D", "Shader", "Prefab", etc.
    std::string source_path;     // Original import path
    u64 lastModified;            // Timestamp for change detection
    std::vector<UUID> dependencies;

    // Type-specific import settings
    nlohmann::json import_settings;

    // Serialization
    void serialize(ISerializatioNode& node) const;
    void deserialize(const ISerializatioNode& node);
};

// Asset registry for editor
class AssetRegistry {
public:
    AssetRegistry(VFS* vfs, const std::string& asset_root);

    // Scanning
    void scan_dir(const std::string& dir, bool recursive = true);
    void refresh();  // Re-scan for changes

    // Lookups
    UUID uuid(const std::string& path) const;
    std::string path(const UUID& uuid) const;
    const AssetMetadata* metadata(const UUID& uuid) const;
    const AssetMetadata* metadata(const std::string& path) const;

    // Modification
    void set_metadata(const UUID& uuid, const AssetMetadata& metadata);
    UUID register_asset(const std::string& path);  // Creates new UUID
    void unregister_asset(const UUID& uuid);

    // Queries
    std::vector<UUID> get_assets_by_type(const std::string& type) const;
    std::vector<UUID> dependencies(const UUID& uuid) const;
    std::vector<UUID> dependents(const UUID& uuid) const;  // Reverse deps

private:
    void load_metadata(const std::string& path);
    void save_metadata(const UUID& uuid);
    std::string metadata_path(const std::string& assetPath);
    
private:
    std::unordered_map<UUID, AssetMetadata> metadata_;
    std::unordered_map<std::string, UUID> path_to_uuid_;
    std::unordered_map<UUID, std::string> uuid_to_path_;
    VFS* vfs_;
    std::string asset_root_;
};
```

**Example .cxmeta sidecar file (JSON):**

```json
{
    "uuid": "550e8400-e29b-41d4-a716-446655440000",
    "type": "Texture2D",
    "source_path": "textures/player/idle.png",
    "last_modified": 1706284800,
    "dependencies": [],
    "import_settings": {
        "filter_mode": "Bilinear",
        "wrap_mode": "Clamp",
        "compression": "DXT5",
        "generate_mipmaps": true,
        "max_size": 2048
    }
}
```

### 5. Asset Manager

```cpp
class AssetManager {
public:
    // Initialization
    void init(const std::string& project_root);
    void dispose();

    // Loading
    template<typename T>
    AssetHandle<T> load(const std::string& path);

    template<typename T>
    AssetHandle<T> load(const UUID& uuid);

    template<typename T>
    cc::Task<void> load_async(const std::string& path);

    // Unloading
    void unload(const UUID& uuid);
    void unload_unused();  // Unload assets with refcount 0

    // Queries
    bool loaded(const UUID& uuid) const;
    template<typename T>
    T* if_loaded(const UUID& uuid);

    // VFS access
    VFS& vfs() { return vfs_; }
    AssetRegistry* registry() { return registry_.get(); }

    // Loader registration
    template<typename T>
    void register_loader(mem::Box<IAssetLoader> loader);

private:
    VFS* vfs_;
    mem::Box<AssetRegistry> registry_;  // Editor only

    // Asset cache (loaded assets)
    std::unordered_map<UUID, Shared<IResource>> loaded_assets_;
    std::unordered_map<UUID, u32> ref_counts_;

    // Asset loaders by type
    std::unordered_map<std::string, Box<IAssetLoader>> loaders_;
};

// Asset loader interface
class IAssetLoader {
public:
    virtual ~IAssetLoader() = default;
    virtual Shared<IResource> load(const std::vector<u8>& data,
                                    const AssetMetadata& metadata) = 0;
    virtual std::string type_name() const = 0;
    virtual std::vector<std::string> extensions() const = 0;
};
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
// codex/file_system/pak_mount.h
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

// Pak builder
class PakBuilder {
private:
    std::vector<std::pair<std::string, std::vector<u8>>> m_Entries;
    CookSettings m_Settings;

public:
    void add_file(const std::string& virtual_path, const std::vector<u8>& data);
    void add_directory(const std::string& virtual_path, const std::string& physical_path);

    void build(const std::string& outputPath);

    // Chunking support
    void set_chunk_assignment(const std::string& path, i32 chunkId);
    void build_chunked(const std::string& outputDir, const std::string& base_name);
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
// PakFlags encode compression per-entry (codex/file_system/cxpak.h)
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

### Phase 2: Asset Registry (Editor) — **PLANNED**
1. Define `.cxmeta` sidecar file format
2. Implement `AssetRegistry` with UUID mapping
3. Directory scanning and metadata caching
4. Dependency tracking between assets

### Phase 3: Asset Manager — **PLANNED**
1. Create `AssetManager` with typed loading API
2. Implement asset caching with ref-counting
3. Register loaders for existing types (Texture2D, Shader)
4. Add support for new types (Prefab, SpriteSheet, Animation)

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
