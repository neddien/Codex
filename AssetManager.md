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

From Simon Coenen's implementation:

```
VFS Architecture:
┌─────────────────────────────────────────┐
│           Virtual File System           │
├─────────────────────────────────────────┤
│  IMountPoint Interface                  │
│  ├── PhysicalMountPoint (directories)   │
│  ├── PakMountPoint (archives)           │
│  └── ZipMountPoint (zip files)          │
├─────────────────────────────────────────┤
│  Priority ordering (higher = preferred) │
│  Enables patching/modding overlays      │
└─────────────────────────────────────────┘
```

**Key Insights:**
- Two-interface design: `IMountPoint` for directory/archive attachment, `File` for individual file operations
- Treats all files uniformly regardless of source (physical, pak, zip, network)
- Priority system allows newer content to override older versions (patching/modding)
- **Same code path for editor and runtime** - just different mount points
- During development, mount physical directories; at runtime, mount pak archives

---

## Recommended Architecture for Codex

### High-Level Overview

```
┌────────────────────────────────────────────────────────┐
│                    AssetManager                         │
│  - AssetHandle<T> Load(AssetPath)                      │
│  - async LoadAsync(AssetPath, callback)                │
│  - Unload(AssetHandle)                                 │
│  - GetMetadata(AssetPath)                              │
├────────────────────────────────────────────────────────┤
│                   Asset Registry                        │
│  - UUID → AssetPath mapping                            │
│  - Dependency graph                                    │
│  - Metadata cache (type, size, dependencies)           │
├────────────────────────────────────────────────────────┤
│                Virtual File System                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ PhysicalMount│  │  PakMount    │  │ MemoryMount  │ │
│  │ (Editor)     │  │ (Runtime)    │  │ (Embedded)   │ │
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
private:
    AssetPath m_Path;
    Shared<T> m_Asset;  // Lazy-loaded
    bool m_Loaded = false;

public:
    AssetHandle() = default;
    AssetHandle(const AssetPath& path);

    // Access operators
    T* operator->() { EnsureLoaded(); return m_Asset.get(); }
    T& operator*() { EnsureLoaded(); return *m_Asset; }

    // State queries
    bool IsLoaded() const { return m_Loaded; }
    bool IsValid() const { return !m_Path.path.empty(); }
    const AssetPath& GetPath() const { return m_Path; }

private:
    void EnsureLoaded();
};
```

### 2. Virtual File System Interface

```cpp
// Mount point interface
class IVFSMount {
public:
    virtual ~IVFSMount() = default;

    // File operations
    virtual bool Exists(const std::string& path) = 0;
    virtual std::vector<u8> Read(const std::string& path) = 0;
    virtual usize GetSize(const std::string& path) = 0;

    // Directory operations
    virtual std::vector<std::string> List(const std::string& dir) = 0;
    virtual bool IsDirectory(const std::string& path) = 0;

    // Mount info
    virtual i32 GetPriority() const = 0;  // Higher = preferred
    virtual std::string GetMountPoint() const = 0;
    virtual std::string GetType() const = 0;  // "physical", "pak", "memory"
};

// Virtual file system
class VirtualFileSystem {
private:
    std::vector<Box<IVFSMount>> m_Mounts;  // Sorted by priority (descending)

public:
    // Mount management
    void Mount(Box<IVFSMount> mount);
    void Unmount(const std::string& mountPoint);
    void SetPriority(const std::string& mountPoint, i32 priority);

    // File operations (tries mounts in priority order)
    bool Exists(const std::string& path);
    std::vector<u8> Read(const std::string& path);
    usize GetSize(const std::string& path);

    // Directory operations
    std::vector<std::string> List(const std::string& dir);
    bool IsDirectory(const std::string& path);

    // Debugging
    std::vector<std::string> GetMountPoints() const;
    IVFSMount* FindMountForPath(const std::string& path);
};
```

### 3. Physical Mount Point (Editor)

```cpp
class PhysicalMount : public IVFSMount {
private:
    std::string m_RootPath;      // Filesystem root (e.g., "/project/assets")
    std::string m_MountPoint;    // Virtual mount point (e.g., "assets")
    i32 m_Priority;

    // Cached directory listing (optional, for performance)
    std::unordered_map<std::string, std::vector<std::string>> m_DirCache;

public:
    PhysicalMount(const std::string& rootPath,
                  const std::string& mountPoint,
                  i32 priority = 0);

    // IVFSMount implementation
    bool Exists(const std::string& path) override;
    std::vector<u8> Read(const std::string& path) override;
    usize GetSize(const std::string& path) override;
    std::vector<std::string> List(const std::string& dir) override;
    bool IsDirectory(const std::string& path) override;
    i32 GetPriority() const override { return m_Priority; }
    std::string GetMountPoint() const override { return m_MountPoint; }
    std::string GetType() const override { return "physical"; }

private:
    std::string ToPhysicalPath(const std::string& virtualPath);
};
```

### 4. Asset Registry (Editor)

```cpp
// Metadata sidecar file structure (.cxmeta)
struct AssetMetadata {
    UUID uuid;
    std::string type;           // "Texture2D", "Shader", "Prefab", etc.
    std::string sourcePath;     // Original import path
    u64 lastModified;           // Timestamp for change detection
    std::vector<UUID> dependencies;

    // Type-specific import settings
    nlohmann::json importSettings;

    // Serialization
    void Serialize(nlohmann::json& j) const;
    void Deserialize(const nlohmann::json& j);
};

// Asset registry for editor
class AssetRegistry {
private:
    std::unordered_map<UUID, AssetMetadata> m_Metadata;
    std::unordered_map<std::string, UUID> m_PathToUUID;
    std::unordered_map<UUID, std::string> m_UUIDToPath;

    VirtualFileSystem* m_VFS;
    std::string m_AssetRoot;

public:
    AssetRegistry(VirtualFileSystem* vfs, const std::string& assetRoot);

    // Scanning
    void ScanDirectory(const std::string& dir, bool recursive = true);
    void Refresh();  // Re-scan for changes

    // Lookups
    UUID GetUUID(const std::string& path) const;
    std::string GetPath(const UUID& uuid) const;
    const AssetMetadata* GetMetadata(const UUID& uuid) const;
    const AssetMetadata* GetMetadata(const std::string& path) const;

    // Modification
    void SetMetadata(const UUID& uuid, const AssetMetadata& metadata);
    UUID RegisterAsset(const std::string& path);  // Creates new UUID
    void UnregisterAsset(const UUID& uuid);

    // Queries
    std::vector<UUID> GetAssetsByType(const std::string& type) const;
    std::vector<UUID> GetDependencies(const UUID& uuid) const;
    std::vector<UUID> GetDependents(const UUID& uuid) const;  // Reverse deps

private:
    void LoadMetadata(const std::string& path);
    void SaveMetadata(const UUID& uuid);
    std::string GetMetadataPath(const std::string& assetPath);
};
```

**Example .cxmeta sidecar file (JSON):**

```json
{
    "uuid": "550e8400-e29b-41d4-a716-446655440000",
    "type": "Texture2D",
    "sourcePath": "textures/player/idle.png",
    "lastModified": 1706284800,
    "dependencies": [],
    "importSettings": {
        "filterMode": "Bilinear",
        "wrapMode": "Clamp",
        "compression": "DXT5",
        "generateMipmaps": true,
        "maxSize": 2048
    }
}
```

### 5. Asset Manager

```cpp
class AssetManager {
private:
    VirtualFileSystem m_VFS;
    Box<AssetRegistry> m_Registry;  // Editor only

    // Asset cache (loaded assets)
    std::unordered_map<UUID, Shared<IResource>> m_LoadedAssets;
    std::unordered_map<UUID, u32> m_RefCounts;

    // Asset loaders by type
    std::unordered_map<std::string, Box<IAssetLoader>> m_Loaders;

public:
    // Initialization
    void Initialize(const std::string& projectRoot);
    void Shutdown();

    // Loading
    template<typename T>
    AssetHandle<T> Load(const std::string& path);

    template<typename T>
    AssetHandle<T> Load(const UUID& uuid);

    template<typename T>
    void LoadAsync(const std::string& path, std::function<void(AssetHandle<T>)> callback);

    // Unloading
    void Unload(const UUID& uuid);
    void UnloadUnused();  // Unload assets with refcount 0

    // Queries
    bool IsLoaded(const UUID& uuid) const;
    template<typename T>
    T* GetIfLoaded(const UUID& uuid);

    // VFS access
    VirtualFileSystem& GetVFS() { return m_VFS; }
    AssetRegistry* GetRegistry() { return m_Registry.get(); }

    // Loader registration
    template<typename T>
    void RegisterLoader(Box<IAssetLoader> loader);
};

// Asset loader interface
class IAssetLoader {
public:
    virtual ~IAssetLoader() = default;
    virtual Shared<IResource> Load(const std::vector<u8>& data,
                                    const AssetMetadata& metadata) = 0;
    virtual std::string GetTypeName() const = 0;
    virtual std::vector<std::string> GetExtensions() const = 0;
};
```

---

## Pak File Format

### Binary Layout

```
┌─────────────────────────────────────────┐
│ Header (fixed size)                     │
│  - Magic number (4 bytes): "CXPK"       │
│  - Version (4 bytes): u32               │
│  - Flags (4 bytes): compression, etc.   │
│  - Entry count (4 bytes): u32           │
│  - TOC offset (8 bytes): u64            │
│  - TOC size (8 bytes): u64              │
│  - Reserved (32 bytes)                  │
├─────────────────────────────────────────┤
│ Data Section                            │
│  [asset data 1] (possibly compressed)   │
│  [asset data 2]                         │
│  [asset data 3]                         │
│  ...                                    │
├─────────────────────────────────────────┤
│ Table of Contents (TOC)                 │
│  For each entry:                        │
│    - Path hash (8 bytes): u64           │
│    - Path string (null-terminated)      │
│    - Data offset (8 bytes): u64         │
│    - Compressed size (8 bytes): u64     │
│    - Uncompressed size (8 bytes): u64   │
│    - Compression type (1 byte): u8      │
│    - Flags (1 byte): u8                 │
│    - CRC32 (4 bytes): u32               │
└─────────────────────────────────────────┘
```

### Pak Mount Implementation

```cpp
struct PakEntry {
    std::string path;
    u64 dataOffset;
    u64 compressedSize;
    u64 uncompressedSize;
    u8 compressionType;  // 0=none, 1=LZ4, 2=ZSTD
    u8 flags;
    u32 crc32;
};

class PakMount : public IVFSMount {
private:
    std::string m_PakPath;
    std::string m_MountPoint;
    i32 m_Priority;

    // Parsed from pak file
    std::unordered_map<u64, PakEntry> m_Entries;  // Hash -> Entry
    std::unordered_map<std::string, u64> m_PathToHash;

    // File handle (kept open)
    std::ifstream m_File;

public:
    PakMount(const std::string& pakPath,
             const std::string& mountPoint,
             i32 priority = 0);
    ~PakMount();

    // IVFSMount implementation
    bool Exists(const std::string& path) override;
    std::vector<u8> Read(const std::string& path) override;
    usize GetSize(const std::string& path) override;
    std::vector<std::string> List(const std::string& dir) override;
    bool IsDirectory(const std::string& path) override;
    i32 GetPriority() const override { return m_Priority; }
    std::string GetMountPoint() const override { return m_MountPoint; }
    std::string GetType() const override { return "pak"; }

private:
    void ParseHeader();
    void ParseTOC();
    std::vector<u8> Decompress(const std::vector<u8>& compressed,
                                u8 compressionType,
                                u64 uncompressedSize);
};
```

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
    virtual std::vector<u8> Cook(const std::vector<u8>& sourceData,
                                  const AssetMetadata& metadata,
                                  const CookSettings& settings) = 0;

    virtual std::string GetTypeName() const = 0;
    virtual std::vector<std::string> GetExtensions() const = 0;
};

// Cook settings
struct CookSettings {
    std::string targetPlatform;  // "windows", "linux", "android"
    bool enableCompression = true;
    u8 compressionType = 1;  // LZ4
    i32 compressionLevel = 6;
    bool stripDebugInfo = true;
    bool generateMipmaps = true;
    i32 maxTextureSize = 4096;
};

// Pak builder
class PakBuilder {
private:
    std::vector<std::pair<std::string, std::vector<u8>>> m_Entries;
    CookSettings m_Settings;

public:
    void AddFile(const std::string& virtualPath, const std::vector<u8>& data);
    void AddDirectory(const std::string& virtualPath, const std::string& physicalPath);

    void Build(const std::string& outputPath);

    // Chunking support
    void SetChunkAssignment(const std::string& path, i32 chunkId);
    void BuildChunked(const std::string& outputDir, const std::string& baseName);
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

### Compression Types

```cpp
enum class CompressionType : u8 {
    None = 0,
    LZ4 = 1,      // Fast decompression, moderate ratio
    ZSTD = 2,     // Good balance of speed and ratio
    LZ4HC = 3,    // Slower compression, same fast decompression
};

// Compression utilities
namespace Compression {
    std::vector<u8> Compress(const std::vector<u8>& data, CompressionType type, i32 level);
    std::vector<u8> Decompress(const std::vector<u8>& data, CompressionType type, u64 uncompressedSize);

    bool ShouldCompress(const std::string& path, u64 size);
}
```

### Security Considerations

From Simon Coenen's article:
- Custom binary format provides basic obscurity
- Data blobs are meaningless without headers
- **Encrypt TOC/headers only** - much faster than encrypting all data
- Optional: per-file encryption for sensitive assets

---

## Implementation Phases

### Phase 1: Foundation
1. Create `VirtualFileSystem` class with mount point interface
2. Implement `PhysicalMount` for directory access
3. Migrate existing `ResourceHandler` to use VFS internally
4. Basic unit tests for VFS operations

### Phase 2: Asset Registry (Editor)
1. Define `.cxmeta` sidecar file format
2. Implement `AssetRegistry` with UUID mapping
3. Directory scanning and metadata caching
4. Dependency tracking between assets

### Phase 3: Asset Manager
1. Create `AssetManager` with typed loading API
2. Implement asset caching with ref-counting
3. Register loaders for existing types (Texture2D, Shader)
4. Add support for new types (Prefab, SpriteSheet, Animation)

### Phase 4: Editor Integration
1. Implement `ContentBrowserView` using VFS and Registry
2. Asset import pipeline with per-type settings
3. File watcher for hot-reload detection
4. Drag-and-drop asset references

### Phase 5: Runtime Packaging
1. Define pak file binary format
2. Implement `PakMount` for reading pak files
3. Create `PakBuilder` tool for creating pak files
4. Integrate with build system (CMake)

### Phase 6: Cooking Pipeline
1. Asset cooker interface and registry
2. Platform-specific texture cooking
3. Compression integration (LZ4/ZSTD)
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
