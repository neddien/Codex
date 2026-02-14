# Codex Engine - Project Context

## Overview

Codex is a modern 2D game engine written in C++20 with an integrated editor. It features ECS-based scene management, Box2D physics, OpenGL rendering, and cross-platform support (Windows, Linux, macOS).

## Project Structure

```
<project-root>/
├── CodexEngine/          # Core engine (shared + static library)
│   ├── src/Engine/       # Engine modules
│   ├── src/Platform/     # Platform abstraction layer
│   └── vendor/           # Header-only deps (glm, entt, stb, glad)
├── CodexEditor/          # ImGui-based editor application
│   └── assets/Projects/  # Project templates
│       ├── TemplateProject/  # Base project template
│       └── MarioClone/       # Example project (copy of TemplateProject)
├── Legacy/               # Abandoned code (ignore)
├── Scripts/              # Build scripts
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
cmake --build --preset linux-any-debug

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
- **Methods**: PascalCase (`OnUpdate`, `GetComponent`)
- **Member variables**: `m_` prefix (`m_Width`, `m_Running`)
- **Static members**: `s_` prefix (`s_Instance`)
- **Enums**: PascalCase (`EventType::WindowClose`)

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

### Core Modules (src/Engine/)

| Module | Purpose |
|--------|---------|
| `Core/` | Application, Window, Layer system, Events, Input |
| `Scene/` | ECS (EnTT), Components, Camera, Physics (Box2D) |
| `Graphics/` | Renderer, BatchRenderer2D, Shaders, Textures |
| `Memory/` | Box, Shared, Ref smart pointers |
| `Reflection/` | Runtime type info for serialization/editor |
| `Serialization/` | Scene/component persistence |
| `System/` | DynamicLibrary, Process management |
| `NativeBehaviour/` | Entity scripting base class |

### Platform Layer (src/Platform/)

| Directory | Purpose |
|-----------|---------|
| `POSIX/` | Unix process management |
| `Linux/` | Linux-specific code |
| `Windows/` | Windows-specific code |
| `OpenGL/` | OpenGL renderer implementation |

### Key Classes

- **Application**: Main loop, layer management, window ownership
- **Scene**: ECS container with EnTT registry, Box2D world
- **Entity**: Wrapper around EnTT entity with component helpers
- **Renderer**: Static facade for rendering operations
- **BatchRenderer2D**: Optimized 2D sprite batching

### Event System
```cpp
// Event dispatch pattern
EventDispatcher dispatcher(event);
dispatcher.Dispatch<WindowCloseEvent>([](WindowCloseEvent& e) {
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
- `NativeBehaviourComponent` - script attachment (uses `m_PendingScripts` for deferred attachment after async compilation)

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
CodexEditor/assets/Projects/TemplateProject/Scripts/reflector.py
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
python Scripts/reflector.py MyScript.h -o ./generated --compile-commands=build
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

### Vendored (in CodexEngine/vendor/)
- GLM - math library
- EnTT - ECS framework
- glad - OpenGL loader
- stb - image loading

## Editor (CodexEditor)

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
- `lgx` (Logex) - logging

### NBMan API
- `NBMan::Load(path, scene)` - loads a compiled script module (.dll/.so) and associates it with a scene
- `NBMan::Unload()` - saves all attached scripts to pending, then unloads the module
- Compilation is async; `pendingNBLoad` flag defers `Load()` to the main thread
- Script attachment lifecycle: deserialize → pending → compile → load → attach

## Common Tasks

### Adding a new component
1. Define struct in `src/Engine/Scene/Components.h`
2. Add reflection metadata with `RF_CLASS`, `RF_PROPERTY`
3. Register serialization in `Serializer`
4. Add to editor PropertiesView if needed

### Adding platform-specific code
1. Create files in appropriate `src/Platform/` subdirectory
2. Add to CMakeLists.txt platform-specific source lists
3. Use `#ifdef CX_PLATFORM_*` guards if needed

### Creating a new NativeBehaviour
1. Create header in project's `src/Scripts/`
2. Inherit from `NativeBehaviour`
3. Add `RF_CLASS()`, `RF_SERIALIZABLE()` macros
4. Mark exposed properties with `RF_PROPERTY()`
5. Run `reflector.py` to generate `.cxr.cpp`
6. Include generated file in build

## Known Issues / TODOs

- [ ] **Abstract Logex behind Engine Logger** - Create `codex::Logger` wrapper to hide `lgx` from NativeBehaviour scripts. Goal: NB scripts should only depend on engine types, not third-party libraries directly. Currently exposed: `glm`, `entt`, `lgx`.

- [ ] SDL audio disabled due to Fedora package issues (using FMOD instead)

- [ ] Some warnings disabled in debug builds

- [ ] CMake install targets reference hardcoded `SDL2` instead of conditional `SDL2-static` on Windows (line 411 in CodexEngine/CMakeLists.txt)

- [ ] `BOX2D_BUILD_TESTBED` is set twice in CodexEngine/CMakeLists.txt (lines 7-8 and 10-11)

- [ ] FMOD paths use `${CMAKE_CURRENT_DIR}` which should be `${CMAKE_CURRENT_SOURCE_DIR}` (line 364)

## Missing Features for 2D Multiplayer Platformer

Analysis conducted to identify gaps for building a networked multiplayer platformer game.

### Feature Status Overview

| Feature | Status | Notes |
|---------|--------|-------|
| **Networking** | MISSING | No infrastructure exists; Legacy/NetNT has abandoned ASIO code |
| **Asset Manager + Prefabs** | MISSING | Only Texture2D/Shader loading; no entity templates |
| **Animation System** | MISSING | SpriteSheet exists but no Animator/state machine |
| **Audio API** | IN PROGRESS | FMOD Studio API 2.02.25 vendored; implementing integration |
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

#### 4. Audio (Done)
- FMOD API 2.02.25 vendored in `/CodexEngine/vendor/fmod/`
- Studio API (bank playback)
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

### Recommended Implementation Order
1. **Audio Integration** - Connect FMOD (IN PROGRESS)
2. **Asset Manager + Prefabs** - Foundation for spawning
3. **Entity Cloning** - Required for prefab instantiation
4. **Animation System** - Visual gameplay
5. **Networking** - Multiplayer core
6. **Platformer Physics** - Ground detection, etc.
7. **Runtime UI** - HUD/menus
8. **Input Action Mapping** - Polish

## Interation Guides
1. **FMOD Audio Integratin** - [Fmod Guide](./Doc/FMODEngine.md)
1. **Asset Manager Integratin** - [Asset Manager Guide](./Doc/AssetManager.md)

## Files to Ignore

- `Legacy/` - abandoned code
- `builds/` - generated build files
- `installs/` - generated install files
- Third-party vendor code (SDL2, Box2D, ImGui, nlohmann/json, FMOD, Logex)
