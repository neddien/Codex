# Codex Engine - Project Context

## Overview

Codex is a modern 2D game engine written in C++20 with an integrated editor. It features ECS-based scene management, Box2D physics, OpenGL rendering, and cross-platform support (Windows, Linux, macOS).

## Project Structure

```
<project-root>/
├── codex_engine/         # Core engine (shared + static library)
│   ├── src/engine/       # Engine modules
│   ├── src/platform/     # Platform abstraction layer
│   └── vendor/           # Header-only deps (glm, entt, stb, glad)
├── codex_editor/         # ImGui-based editor application
│   └── assets/Projects/  # Project templates
│       ├── TemplateProject/  # Base project template
│       └── MarioClone/       # Example project (copy of TemplateProject)
├── legacy/               # Abandoned code (ignore)
├── scripts/              # Build scripts
├── builds/               # Build outputs (generated)
└── installs/             # Install outputs (generated)
```

## Build System

Uses **CMake Presets** (CMakePresets.json). Do not use raw cmake commands.

### Available Presets

| Preset | Platform | Build Type |
|--------|----------|------------|
| `linux-any-debug` | Linux | Debug |
| `linux-any-release` | Linux | Release (with sanitizers) |
| `linux-any-shipping` | Linux | Shipping |
| `windows-msvc-any-debug` | Windows | Debug (MSVC) |
| `windows-msvc-any-release` | Windows | Release (MSVC) |
| `windows-llvm-any-debug` | Windows | Debug (Clang) |
| `osx-any-debug` | macOS | Debug |
| `osx-any-release` | macOS | Release |
| `vs2022` | Windows | Visual Studio 2022 |

### Build Commands

```bash
# Configure (pick appropriate preset for your platform)
cmake --preset linux-any-debug

# Build
cmake --build builds/linux-any-debug

# Install
cmake --install builds/linux-any-debug
```

### Platform Macros
- `CX_PLATFORM_LINUX`, `CX_PLATFORM_WINDOWS`, `CX_PLATFORM_OSX`
- `CX_PLATFORM_UNIX` (Linux + OSX)
- `CX_COMPILER_GNUC`, `CX_COMPILER_CLANG`, `CX_COMPILER_MSVC`
- `CX_BUILD_TYPE_DEBUG`, `CX_BUILD_TYPE_RELEASE`, `CX_BUILD_TYPE_SHIPPING`

## Coding Conventions

### Naming
- **Classes**: PascalCase (`Application`, `Entity`, `Scene`)
- **Methods**: snake_case (`on_update`, `get_component`)
- **Member variables**: trailing `_` suffix (`width_`, `running_`)
- **Static members**: `s_` prefix + trailing `_` (`s_instance_`)
- **Enums**: snake_case with `enum class` (`event_type::window_close`)
- **Getters**: STL-style (`foo()` not `get_foo()`); **Setters**: `set_foo()`

### Type Aliases (from CommonDef.h)
- `u8`, `u16`, `u32`, `u64` - unsigned integers
- `i8`, `i16`, `i32`, `i64` - signed integers
- `f32`, `f64`, `f128` - floats
- `usize` - size type
- `object` - void pointer

### Memory Management
**Never use raw pointers or std smart pointers in public API.** Use:
- `Box<T>` - unique ownership (like unique_ptr)
- `Shared<T>` - shared ownership (like shared_ptr)
- `Ref<T>` - weak reference

### Macros
- `CODEX_API` - export/visibility control
- `CX_ASSERT(expr)` - debug assertion
- `CX_CUSTOM_EXCEPTION(name)` - define exception type

## Engine Architecture

### Core Modules (src/engine/)

| Module | Purpose |
|--------|---------|
| `core/` | Engine (main loop/singleton), Window, Layer system, Events, Input |
| `scene/` | ECS (EnTT), Components, Camera, Physics (Box2D) |
| `graphics/` | Renderer, BatchRenderer2D, Shaders, Textures |
| `memory/` | Box, Shared, Ref smart pointers |
| `concurrency/` | ThreadPool, Task\<T\> (coroutine), ThreadedExecutor, CooperativeExecutor |
| `reflection/` | Runtime type info for serialization/editor |
| `system/` | DynamicLibrary, Process management |
| `native_behaviour/` | Entity scripting base class |
| `file_system/` | VFS (trie-based), DiskMount, MemoryMount, PakMount, FileHandle |
| `audio/` | FMOD audio integration |

### Platform Layer (src/platform/)

| Directory | Purpose |
|-----------|---------|
| `posix/` | Unix process management |
| `linux/` | Linux-specific file handles (`LinuxFileHandle`) |
| `windows/` | Windows-specific code (`NtFileHandle`) |
| `open_gl/` | OpenGL renderer implementation |

### Key Classes

- **Engine** (was `Application`): Main loop, layer management, window ownership. Factory: `create_engine(EngineArgs)`. Config: `EngineProperties`. Singleton access: `Engine::get()`.
- **Scene**: ECS container with EnTT registry, Box2D world
- **Entity**: Wrapper around EnTT entity with component helpers
- **Renderer**: Static facade for rendering operations
- **BatchRenderer2D**: Optimized 2D sprite batching

### Event System
```cpp
// Event dispatch pattern
EventDispatcher dispatcher{ event };
dispatcher.dispatch<WindowCloseEvent>([](WindowCloseEvent& e) {
    // handle
    return true;
});
```

### Component System
Components are POD structs registered with EnTT:
- `TransformComponent` - position, rotation, scale
- `SpriteComponent` - texture, color, atlas coords
- `RigidBody2DComponent` - Box2D body wrapper
- `BoxCollider2DComponent` - Box2D collider
- `CameraComponent` - scene camera
- `NativeBehaviourComponent` - script attachment (uses `pending_scripts_` for deferred attachment after async compilation)

## Abbreviations

| Abbreviation | Meaning |
|--------------|---------|
| `CX` | Codex (engine prefix) |
| `CE` | Codex Editor |
| `NB` | NativeBehaviour |
| `NBMan` | NativeBehaviour Manager |
| `RF` | Reflector/Reflection |
| `lgx` | Logex (logging library namespace) |

## Reflection System & reflector.py

The engine uses a custom reflection system for NativeBehaviour (NB) scripts. The reflection generator is located at:
```
codex_editor/assets/Projects/TemplateProject/scripts/reflector.py
```

### How It Works

1. Uses **libclang** (python-clang) to parse C++ headers
2. Looks for classes annotated with `RF_CLASS` macro
3. Finds properties marked with `RF_PROPERTY`
4. Generates `.cxr.cpp` files with:
   - `Serialize()` / `Deserialize()` implementations
   - `GetTypeInfo()` for runtime reflection
   - Static type registration with `NBMan`

### Usage

**You typically don't run reflector.py manually.** The NBMan CMake project automatically invokes it during the build process. Users only need to:

1. Add reflection macros to their NB scripts
2. Build the project - reflection code is generated automatically

For manual invocation (if needed):
```bash
python scripts/reflector.py my_script.h -o ./generated --compile-commands=build
```

### Reflection Macros
```cpp
RF_CLASS()
class MyScript : public NativeBehaviour {
    RF_SERIALIZABLE()

    RF_PROPERTY()
    float m_Speed = 5.0f;

    RF_PROPERTY(DisplayName = "Health Points", Category = "Stats")
    int m_Health = 100;
};
```

### Supported Property Types
- Primitives: `int`, `float`, `double`, `bool`
- Strings: `std::string`
- Vectors: `Vector2`, `Vector3`, `Vector4` (and `f` variants)
- Arrays: `std::vector<T>`

## Dependencies

### Third-Party (do not modify)
- SDL2 2.32.x - windowing/input
- Box2D 2.4.2 - 2D physics
- ImGui - editor UI
- nlohmann/json 3.12.0 - serialization
- FMOD 13.25 - audio
- Logex - logging (custom, but treat as third-party)

### Vendored (in codex_engine/vendor/)
- GLM - math library
- EnTT - ECS framework
- glad - OpenGL loader
- stb - image loading

## Editor (codex_editor)

ImGui-based editor with panels:
- **SceneEditorView** - main viewport with gizmos (ImGuizmo)
- **SceneHierarchyView** - entity tree
- **PropertiesView** - component inspector
- **ContentBrowserView** - asset browser
- **ToolbarView** - play/pause controls

## NativeBehaviour (NB) Scripts

Native C++ scripts that attach to entities. Currently exposed third-party libraries:
- `glm` - math
- `entt` - ECS (direct registry access)

Logging is available via `NativeBehaviour`'s protected methods (`info`, `warn`, `error`, `fatal`, `trace`) — routes to the nbman logger. Scripts no longer need to use `lgx` directly.

### NBMan API
- `NBMan::load(path, scene)` - loads a compiled script module (.dll/.so) and associates it with a scene
- `NBMan::unload()` - saves all attached scripts to pending, then unloads the module
- Compilation is async; `pending_nb_load` atomic flag defers `load()` to the main thread
- Script attachment lifecycle: deserialize → pending → compile → load → attach

## Common Tasks

### Adding a new component
1. Define struct in `src/engine/scene/Components.h`
2. Add reflection metadata with `RF_CLASS`, `RF_PROPERTY`
3. Register serialization in `Serializer`
4. Add to editor PropertiesView if needed

### Adding platform-specific code
1. Create files in appropriate `src/platform/` subdirectory
2. Add to CMakeLists.txt platform-specific source lists
3. Use `#ifdef CX_PLATFORM_*` guards if needed

### Creating a new NativeBehaviour
1. Create header in project's `src/scripts/`
2. Inherit from `NativeBehaviour`
3. Add `RF_CLASS()`, `RF_SERIALIZABLE()` macros
4. Mark exposed properties with `RF_PROPERTY()`
5. Run `reflector.py` to generate `.cxr.cpp`
6. Include generated file in build

## Known Issues / TODOs

- [x] **Abstract Logex behind Engine Logger** - Three `lgx::Logger` instances (`engine`, `editor`, `nbman`) managed by `Engine`. Free functions `codex::info/warn/error/fatal/trace` in `log.h`. `NativeBehaviour` has protected logging methods. Scripts no longer expose `lgx` directly.

- [ ] SDL audio disabled due to Fedora package issues (using FMOD instead)

- [ ] Some warnings disabled in debug builds

- [ ] CMake install targets reference hardcoded `SDL2` instead of conditional `SDL2-static` on Windows (line 411 in codex_engine/CMakeLists.txt)

- [ ] `BOX2D_BUILD_TESTBED` is set twice in codex_engine/CMakeLists.txt (lines 7-8 and 10-11)

- [ ] FMOD paths use `${CMAKE_CURRENT_DIR}` which should be `${CMAKE_CURRENT_SOURCE_DIR}` (line 364)

## Missing Features for 2D Multiplayer Platformer

Analysis conducted to identify gaps for building a networked multiplayer platformer game.

### Feature Status Overview

| Feature | Status | Notes |
|---------|--------|-------|
| **Networking** | MISSING | No infrastructure exists; Legacy/NetNT has abandoned ASIO code |
| **Asset Manager + Prefabs** | MISSING | Only Texture2D/Shader loading; no entity templates |
| **Animation System** | MISSING | SpriteSheet exists but no Animator/state machine |
| **Audio API** | DONE | FMOD Studio API integrated; bank playback via AudioSystem |
| **Virtual Filesystem** | DONE | VFS, DiskMount, MemoryMount, PakMount (.cxpak with LZ4) implemented |
| **Runtime UI** | MISSING | Only editor ImGui; no in-game HUD system |
| **Platformer Physics** | PARTIAL | Box2D works; missing ground detection, one-way platforms |
| **Input Action Mapping** | MISSING | Raw keys only; no rebindable actions |
| **Entity Cloning** | MISSING | Only full scene copy; no Entity::Clone() |

### Detailed Gap Analysis

#### 1. Networking (Critical)
- Zero network code in active codebase
- Need: Transport layer (ENet/GameNetworkingSockets), message protocol, state sync, lag compensation

#### 2. Asset Manager + Prefabs (Critical)
- `ResourceHandler.h` only supports Texture2D and Shader
- No entity prefab/template system
- No `Scene::InstantiatePrefab()` or `Entity::Clone()`
- `ContentBrowserView` is a stub (empty window)
- `PrefabFile` enum defined but unimplemented
- **Impact**: Cannot spawn players/enemies/projectiles from templates

#### 3. Animation System (Critical)
- `SpriteSheet.h` provides static sprite extraction only
- No `AnimationClip`, `AnimatorComponent`, or state machine
- No frame playback, transitions, or animation events

#### 4. Audio (DONE)
- FMOD API 2.02.25 vendored in `codex_engine/vendor/fmod/`
- `AudioSystem` + `AudioManager` integrated; bank loading and event playback
- `ax::EventHandle` exposed to NativeBehaviour scripts via `get_audio_event(event_path)`
- SDL audio explicitly disabled (using FMOD instead)

#### 5. Platformer Physics Helpers (Important)
- Box2D integrated with RigidBody2D, BoxCollider2D, CircleCollider2D
- Missing: Ground detection, one-way platforms, moving platforms, collision callbacks

#### 6. Runtime UI (Important)
- Only editor ImGui available
- No Canvas/Widget system for in-game HUD

#### 7. Input Action Mapping (Nice-to-have)
- `Input.h` has complete key enumeration (277 keys)
- Missing: Action definitions, rebindable bindings, input buffering

### What's Production-Ready
- BatchRenderer2D with sprite Z-ordering
- EnTT-based ECS with component serialization
- Scene persistence (full scene save/load)
- Input handling (keyboard/mouse)
- Box2D physics simulation
- NativeBehaviour scripting with reflection
- Cross-platform build system
- Virtual Filesystem (VFS) with disk, memory, and PAK archive mounts (LZ4 compression, CRC integrity)
- FMOD audio integration (bank playback via AudioSystem)
- Engine logging system (engine/editor/nbman loggers, `codex::info/warn/error` free functions)

### Recommended Implementation Order
1. ~~**Audio Integration**~~ - DONE
2. ~~**Virtual Filesystem**~~ - DONE
3. **Asset Manager + Prefabs** - Foundation for spawning
4. **Entity Cloning** - Required for prefab instantiation
5. **Animation System** - Visual gameplay
6. **Networking** - Multiplayer core
7. **Platformer Physics** - Ground detection, etc.
8. **Runtime UI** - HUD/menus
9. **Input Action Mapping** - Polish

## Integration Guides
1. **FMOD Audio Integration** - [FMOD Guide](./doc/FMODEngine.md)
1. **Asset Manager Integration** - [Asset Manager Guide](./doc/AssetManager.md)
1. **Virtual Filesystem (VFS / PAK)** - [FileSystem Guide](./doc/FileSystem.md)

## Files to Ignore

- `legacy/` - abandoned code
- `builds/` - generated build files
- `installs/` - generated install files
- Third-party vendor code (SDL2, Box2D, ImGui, nlohmann/json, FMOD, Logex)
