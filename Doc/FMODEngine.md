# FMOD Audio Integration

## Overview

Codex Engine uses FMOD for audio playback, supporting both direct audio file playback (Core API) and FMOD Studio bank-based events (Studio API).

**FMOD Version:** 2.02.25
**Location:** `CodexEngine/vendor/fmod/fmodstudioapi20225linux/`

## Architecture Decision: Option C (Both APIs)

| Core API | Studio API |
|----------|------------|
| Direct file playback (WAV, MP3, OGG) | Bank-based event playback |
| Quick prototyping | Polished, adaptive audio |
| No external tooling required | Requires FMOD Studio authoring |
| Simple one-shots, procedural audio | Complex layered sounds, music |

## FMOD Concepts

### Core API (`libfmod.so`)

Low-level audio engine:
- **System** - Main entry point, manages devices and resources
- **Sound** - Loaded audio data (can be streamed or in-memory)
- **Channel** - Playing instance of a sound
- **ChannelGroup** - Mixer bus for grouping channels
- **DSP** - Audio effects (reverb, echo, EQ, custom)

```cpp
// Core API usage
FMOD::System* system;
FMOD::Sound* sound;
FMOD::Channel* channel;

FMOD::System_Create(&system);
system->init(512, FMOD_INIT_NORMAL, nullptr);
system->createSound("jump.wav", FMOD_DEFAULT, nullptr, &sound);
system->playSound(sound, nullptr, false, &channel);
channel->setVolume(0.8f);
```

### Studio API (`libfmodstudio.so`)

High-level system for designed audio:
- **Studio::System** - Wraps Core system, manages banks
- **Bank** - Container for events and audio data (.bank files)
- **EventDescription** - Template/definition of an event
- **EventInstance** - Playing instance of an event
- **Parameter** - Runtime variable affecting sound behavior
- **Bus** - Mixer channel for routing and effects
- **VCA** - Volume control aggregate (links multiple buses)
- **Snapshot** - Mixer state preset (e.g., "underwater", "paused")

```cpp
// Studio API usage
FMOD::Studio::System* studioSystem;
FMOD::Studio::Bank* bank;
FMOD::Studio::EventDescription* eventDesc;
FMOD::Studio::EventInstance* eventInst;

FMOD::Studio::System::create(&studioSystem);
studioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
studioSystem->loadBankFile("Master.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
studioSystem->getEvent("event:/Player/Footstep", &eventDesc);
eventDesc->createInstance(&eventInst);
eventInst->setParameterByName("surface", 1.0f);
eventInst->start();
```

## FMOD Studio Workflow

FMOD Studio is a standalone authoring tool (not shipped with games).

```
FMOD Studio (Authoring)              Game Runtime
+--------------------------+         +------------------------+
| 1. Import raw audio      |         | 1. Load .bank files    |
| 2. Create Events         |  --->   | 2. Lookup events       |
| 3. Add automation/params |  .bank  | 3. Set parameters      |
| 4. Configure mixer       |  files  | 4. Play/stop events    |
| 5. Build banks           |         | 5. Update each frame   |
+--------------------------+         +------------------------+
```

### Bank Files

| File | Contents |
|------|----------|
| `Master.bank` | Event metadata, mixer routing, required |
| `Master.strings.bank` | Event path strings for lookup by name |
| `SFX.bank` | Audio data for SFX events |
| `Music.bank` | Audio data for music events |

Banks can be split by category for streaming/memory management.

### Events

An Event is a designed sound unit that can contain:
- Multiple audio files (randomly selected or layered)
- Automation curves (volume, pitch, effects over time)
- Parameters (game-driven variables)
- Conditions and logic

**Example:** "Footstep" event
- 10 footstep samples (random selection)
- `surface` parameter: 0=concrete, 1=grass, 2=metal (switches sample set)
- `speed` parameter: affects pitch
- Scatterer: randomizes timing slightly
- Cooldown: prevents rapid-fire playback

### Parameters

Runtime variables set by game code:
- **Continuous:** 0.0 - 1.0 (e.g., health, speed, distance)
- **Discrete/Labeled:** Named values (e.g., surface type)
- **Global:** Shared across all events (e.g., time of day)
- **Local:** Per-event instance

## Planned Engine Integration

### File Structure

```
CodexEngine/src/Engine/Audio/
├── Public/
│   ├── Audio.h              # Main API facade
│   ├── AudioSystem.h        # System singleton
│   ├── Sound.h              # Core API sound wrapper
│   └── AudioEvent.h         # Studio API event wrapper
├── AudioSystem.cpp
├── Sound.cpp
└── AudioEvent.cpp

CodexEngine/src/Engine/Scene/
└── Components.h             # + AudioSourceComponent, AudioListenerComponent
```

### Public API Design

```cpp
namespace codex::audio {

// Simple playback (Core API) - no FMOD Studio needed
class Audio {
public:
    // One-shot playback
    static void PlaySound(const std::filesystem::path& path);
    static void PlaySound(const std::filesystem::path& path, float volume);
    static void PlaySound(const std::filesystem::path& path, const Vector3& position);

    // Controlled playback
    static SoundHandle Play(const std::filesystem::path& path);

    // Event playback (Studio API) - requires .bank
    static void PlayEvent(const std::string& eventPath);
    static EventHandle Play(const std::string& eventPath);

    // Bank management
    static void LoadBank(const std::filesystem::path& bankPath);
    static void UnloadBank(const std::filesystem::path& bankPath);

    // Global controls
    static void SetMasterVolume(float volume);
    static void PauseAll();
    static void ResumeAll();
    static void StopAll();
};

// Handle for controlling playing sound (Core API)
class SoundHandle {
public:
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetPosition(const Vector3& position);
    void SetPaused(bool paused);
    void Stop();
    bool IsPlaying() const;
};

// Handle for controlling event instance (Studio API)
class EventHandle {
public:
    void SetParameter(const std::string& name, float value);
    float GetParameter(const std::string& name) const;
    void SetVolume(float volume);
    void SetPosition(const Vector3& position);
    void SetPaused(bool paused);
    void Start();
    void Stop(bool allowFadeout = true);
    bool IsPlaying() const;
};

} // namespace codex::audio
```

### ECS Components

```cpp
// Attach sound/event to entity with 3D positioning
struct AudioSourceComponent {
    std::string eventPath;           // "event:/SFX/Explosion" or empty for sound
    std::filesystem::path soundPath; // "assets/audio/boom.wav" or empty for event

    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    bool playOnStart = false;
    bool is3D = true;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;

    // Runtime state (not serialized)
    std::variant<SoundHandle, EventHandle> m_Handle;
};

// Usually attached to camera entity
struct AudioListenerComponent {
    // Position/orientation taken from TransformComponent
    // Only one active listener at a time
};
```

### AudioSystem Lifecycle

```cpp
class AudioSystem {
public:
    static void Init();      // Called by Application::Init()
    static void Update();    // Called every frame (required by FMOD)
    static void Shutdown();  // Called by Application::Shutdown()

    // Access underlying FMOD systems (for advanced use)
    static FMOD::System* GetCoreSystem();
    static FMOD::Studio::System* GetStudioSystem();
};
```

## CMake Integration

### Required Changes

```cmake
# In CodexEngine/CMakeLists.txt

# FMOD paths
set(FMOD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/fmod/fmodstudioapi20225linux)
set(FMOD_CORE_INC ${FMOD_DIR}/api/core/inc)
set(FMOD_STUDIO_INC ${FMOD_DIR}/api/studio/inc)
set(FMOD_CORE_LIB ${FMOD_DIR}/api/core/lib/x86_64)
set(FMOD_STUDIO_LIB ${FMOD_DIR}/api/studio/lib/x86_64)

# Include directories
target_include_directories(CodexEngine-shared PUBLIC
    ${FMOD_CORE_INC}
    ${FMOD_STUDIO_INC}
)

# Link libraries
target_link_libraries(CodexEngine-shared PUBLIC
    ${FMOD_CORE_LIB}/libfmod.so
    ${FMOD_STUDIO_LIB}/libfmodstudio.so
)

# For debug builds, use logging versions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(CodexEngine-shared PUBLIC
        ${FMOD_CORE_LIB}/libfmodL.so
        ${FMOD_STUDIO_LIB}/libfmodstudioL.so
    )
endif()
```

### Platform-Specific Libraries

| Platform | Core Library | Studio Library |
|----------|--------------|----------------|
| Linux | `libfmod.so` | `libfmodstudio.so` |
| Windows | `fmod.dll` / `fmod_vc.lib` | `fmodstudio.dll` / `fmodstudio_vc.lib` |
| macOS | `libfmod.dylib` | `libfmodstudio.dylib` |

**Note:** Currently only Linux SDK is vendored. Windows/macOS SDKs need to be downloaded from fmod.com.

## Implementation Phases

### Phase 1: Foundation
- [ ] Fix CMake FMOD path (`CMAKE_CURRENT_DIR` -> `CMAKE_CURRENT_SOURCE_DIR`)
- [ ] Link FMOD Core and Studio libraries
- [ ] Create `AudioSystem` singleton with Init/Update/Shutdown
- [ ] Verify FMOD initializes without errors

### Phase 2: Core API Playback
- [ ] Implement `Sound` wrapper class
- [ ] Implement `SoundHandle` for playback control
- [ ] Add `Audio::PlaySound()` static methods
- [ ] Test with WAV/OGG files

### Phase 3: Studio API Playback
- [ ] Implement bank loading
- [ ] Implement `AudioEvent` wrapper class
- [ ] Implement `EventHandle` for event control
- [ ] Add `Audio::PlayEvent()` static methods
- [ ] Test with sample .bank file

### Phase 4: 3D Audio
- [ ] Add listener position/orientation tracking
- [ ] Add 3D attributes to sounds and events
- [ ] Implement distance attenuation

### Phase 5: ECS Integration
- [ ] Add `AudioSourceComponent`
- [ ] Add `AudioListenerComponent`
- [ ] Update audio positions from transforms each frame
- [ ] Add serialization support
- [ ] Add editor PropertiesView support

### Phase 6: NativeBehaviour API
- [ ] Expose audio API to scripts
- [ ] Add to NB public headers

## FMOD Licensing

FMOD has a tiered licensing model:
- **Free:** Budget under $200k USD
- **Indie:** Budget $200k - $500k
- **Commercial:** Budget above $500k

Attribution required. See: https://fmod.com/legal

## Resources

- [FMOD API Documentation](https://fmod.com/docs/2.02/api/)
- [FMOD Studio Documentation](https://fmod.com/docs/2.02/studio/)
- [FMOD Core API Reference](https://fmod.com/docs/2.02/api/core-api.html)
- [FMOD Studio API Reference](https://fmod.com/docs/2.02/api/studio-api.html)
