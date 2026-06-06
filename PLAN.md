# CNA Samples — Migration Plan

## Overview

This repository contains C++ ports of the official **Microsoft XNA Game Studio 4.0** sample collection,
migrated to run on [CNA](https://github.com/openeggbert/cna) — a C++ reimplementation of the XNA 4.0
programming model built on SDL3 and a pluggable graphics backend.

The original C# samples are archived at `/tmp/XNAGameStudio/Samples` (local mirror of
[XNA Game Studio archive](https://github.com/SimonDarksideJ/XNAGameStudio)).

---

## Goals

1. Port every applicable XNA 4.0 desktop sample to C++ using the CNA framework API.
2. Preserve the original XNA 4.0 class hierarchy and naming (`Microsoft::Xna::Framework::*`).
3. Build cleanly on Linux, Windows, and macOS via CMake (≥ 3.20) and C++23.
4. Serve as living integration tests and usage examples for the CNA framework itself.
5. Provide a reference for developers migrating XNA/MonoGame code to CNA.

---

## Repository Structure

```
cna-samples/
├── CMakeLists.txt          # Root build; finds CNA, registers each sample subdirectory
├── CMakePresets.json       # Convenience presets (debug, release, web/Emscripten)
├── cmake/
│   └── CopySampleContent.cmake   # Helper: copy Content/ assets next to built executable
├── samples/
│   ├── Platformer/         # Full platformer game (2D sprites, audio, levels)
│   ├── PrimitivesSample/   # 2D primitive batch (lines, points, spacewar retro mode)
│   ├── Primitives3D/       # 3D primitives (cube, sphere, cylinder, torus, teapot)
│   ├── BloomSample/        # Post-process bloom effect
│   ├── CollisionSample/    # Per-pixel 2D collision
│   ├── SpriteBatch/        # SpriteBatch basics (textures, transforms, effects)
│   ├── SpriteEffects/      # SpriteEffects (flip, color, rotation)
│   ├── SpriteSheet/        # Sprite sheet / animation
│   ├── Audio3D/            # 3D positional audio with AudioEmitter/AudioListener
│   ├── SoundAndMusic/      # Background music + SFX
│   ├── InputReporter/      # Keyboard, gamepad, touch input reporting
│   ├── InputSequence/      # Combo/sequence detection
│   ├── ChaseCamera/        # 3D chase camera following a model
│   ├── HeightmapCollision/ # Terrain height-map + collision
│   ├── Pathfinding/        # A* pathfinding on a tile grid
│   ├── FlockingSample/     # Boids flocking simulation
│   ├── ParticleSample/     # CPU particle system
│   ├── ShadowMapping/      # Shadow mapping with render targets
│   ├── NormalMapping/      # Normal / bump mapping shader
│   ├── SkinningSample/     # GPU skinned model animation
│   ├── MarbleMaze/         # Full marble-maze game (3D physics)
│   ├── Spacewar/           # Classic Spacewar game
│   ├── RolePlayingGame/    # Top-down RPG (map, dialogue, combat)
│   ├── NetRumble/          # Multiplayer arcade shooter
│   └── ... (see Sample Inventory below)
├── .gitignore
├── LICENSE                 # Microsoft Permissive License (Ms-PL) — matches XNA originals
├── NOTICE.md               # Third-party attribution
├── PLAN.md                 # This file
└── README.md
```

Each sample subdirectory follows this layout:

```
samples/SampleName/
├── CMakeLists.txt          # add_executable, link CNA, copy assets
├── src/
│   ├── Program.cpp         # int main() — CNA/Entrypoint.hpp included here
│   ├── SampleGame.cpp      # Derived from Microsoft::Xna::Framework::Game
│   └── *.cpp / *.hpp       # Additional game classes
└── Content/                # Assets: textures, audio, fonts, shaders (XNB or raw)
```

---

## Build System

### Dependencies

| Dependency | Version | Notes |
|---|---|---|
| CMake | ≥ 3.20 | Required |
| C++ compiler | C++23 | GCC 13+, Clang 16+, MSVC 19.38+ |
| CNA | `../cna` (sibling directory) | Provides `CNA` CMake target |
| sharp-runtime | `../sharp-runtime` (via CNA) | Pulled in transitively |

### Integration with CNA

`cna-samples` consumes CNA as an `add_subdirectory(../cna CNA_BUILD)` dependency.
This matches the pattern used inside the CNA repository itself for its own examples.

CMake cache variables forwarded to CNA:

| Variable | Value |
|---|---|
| `CNA_BUILD_TESTS` | `OFF` |
| `CNA_BUILD_EXAMPLES` | `OFF` |
| `SHARP_RUNTIME_BUILD_TESTS` | `OFF` |

### CMake Targets per Sample

Each sample `SampleName` produces a single executable target named `cna_sample_<lowercase_name>`.

Example: `cna_sample_platformer`, `cna_sample_primitives3d`.

### Linking Pattern

Follows the same linker-group pattern as CNA's own examples to handle circular references
between the CNA static library and the EasyGL backend:

```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32)
    target_link_libraries(cna_sample_foo PRIVATE
        -Wl,--start-group CNA ${CNA_BACKEND_TARGET} -Wl,--end-group
        SHARP_RUNTIME)
else()
    target_link_libraries(cna_sample_foo PRIVATE CNA SHARP_RUNTIME)
endif()
```

---

## Migration Strategy (C# → C++)

### Namespace Mapping

| C# | C++ |
|---|---|
| `Microsoft.Xna.Framework` | `Microsoft::Xna::Framework` |
| `Microsoft.Xna.Framework.Graphics` | `Microsoft::Xna::Framework::Graphics` |
| `Microsoft.Xna.Framework.Input` | `Microsoft::Xna::Framework::Input` |
| `Microsoft.Xna.Framework.Audio` | `Microsoft::Xna::Framework::Audio` |
| `Microsoft.Xna.Framework.Content` | `Microsoft::Xna::Framework::Content` |
| `Microsoft.Xna.Framework.Media` | `Microsoft::Xna::Framework::Media` |

### Class Mapping (key types)

| C# class | C++ equivalent |
|---|---|
| `Game` | `Microsoft::Xna::Framework::Game` |
| `GraphicsDeviceManager` | `Microsoft::Xna::Framework::GraphicsDeviceManager` |
| `SpriteBatch` | `Microsoft::Xna::Framework::Graphics::SpriteBatch` |
| `Texture2D` | `Microsoft::Xna::Framework::Graphics::Texture2D` |
| `SpriteFont` | `Microsoft::Xna::Framework::Graphics::SpriteFont` |
| `GameTime` | `Microsoft::Xna::Framework::GameTime` |
| `KeyboardState` | `Microsoft::Xna::Framework::Input::KeyboardState` |
| `GamePadState` | `Microsoft::Xna::Framework::Input::GamePadState` |
| `Vector2 / Vector3` | `Microsoft::Xna::Framework::Vector2 / Vector3` |
| `Color` | `Microsoft::Xna::Framework::Color` |
| `Rectangle` | `Microsoft::Xna::Framework::Rectangle` |
| `ContentManager` | `Microsoft::Xna::Framework::Content::ContentManager` |

### Language Idiom Conversions

| C# idiom | C++ idiom |
|---|---|
| `class Foo : Game` | `class Foo : public Microsoft::Xna::Framework::Game` |
| `override void Update(GameTime gt)` | `void Update(const GameTime& gt) override` |
| `List<T>` | `std::vector<T>` |
| `Dictionary<K,V>` | `std::unordered_map<K,V>` |
| `string` | `std::string` |
| `new Foo(...)` | `std::make_unique<Foo>(...)` or stack allocation |
| `#region ... #endregion` | removed (not applicable) |
| Properties (`get; set;`) | member variables or accessor methods |
| `foreach (var x in list)` | range-for `for (auto& x : list)` |
| `Math.Sin(x)` | `std::sin(x)` |
| `Random` | `std::mt19937` / `std::uniform_real_distribution` |
| `TimeSpan` | `sharp_runtime::TimeSpan` (from sharp-runtime) |
| `int?` / nullable | `std::optional<int>` |
| `event EventHandler<T>` | CNA event mechanism or `std::function` callbacks |
| Content pipeline (`.xnb`) | CNA ContentManager loading or raw asset loading |

### Content / Assets

XNA uses compiled `.xnb` binary assets. CNA's `ContentManager` either:
- Loads raw assets (PNG, OGG, WAV, etc.) with automatic format detection, **or**
- Uses a lightweight CNA content pipeline that processes source assets at build time.

For each sample, the `Content/` directory is copied next to the built executable via
a post-build CMake command (`cmake -E copy_directory`).

---

## Sample Inventory and Priority

### Phase 1 — Foundation (implement first, these validate core CNA APIs)

| Sample | XNA Source Dir | Key APIs Exercised |
|---|---|---|
| **PrimitivesSample** | `PrimitivesSample_4_0` | PrimitiveBatch, Vector2, Color, Game loop |
| **Primitives3D** | `Primitives3DSample_4_0` | 3D primitives, BasicEffect, VertexBuffer |
| **SpriteBatch basics** | `ReachGraphicsDemo_4_0` | SpriteBatch, Texture2D, SpriteFont |
| **SpriteEffects** | `SpriteEffectsSample_4_0` | SpriteEffects enum, SpriteBatch.Draw |
| **SpriteSheet** | `SpriteSheetSample_4_0` | Animated sprites, source rectangles |
| **TexturesAndColors** | `TexturesAndColorsSample_4_0` | Texture2D creation, color math |
| **InputReporter** | `InputReporter_4_0` | Keyboard, GamePad, Touch input |

### Phase 2 — 2D Games

| Sample | XNA Source Dir |
|---|---|
| **Platformer** | `Platformer_4_0` |
| **CollisionSample** | `CollisionSample_4_0` |
| **PerPixelCollision** | `PerPixelCollisionSample_4_0` |
| **RectangleCollision** | `RectangleCollisionSample_4_0` |
| **PathDrawing** | `PathDrawing_4_0` |
| **Pathfinding** | `Pathfinding_4_0` |
| **FlockingSample** | `FlockingSample_4_0` |
| **Spacewar** | `Spacewar_4_0` |
| **TicTacToe** | `TicTacToe_4_0` |
| **ParticleSample** | `ParticleSample_4_0` |

### Phase 3 — 3D Graphics

| Sample | XNA Source Dir |
|---|---|
| **ChaseCamera** | `ChaseCamera_4_0` |
| **HeightmapCollision** | `HeightmapCollisionSample_4_0` |
| **BloomSample** | `BloomSample_4_0` |
| **ShadowMapping** | `ShadowMappingSample_4_0` |
| **NormalMapping** | `NormalMappingSample_4_0` |
| **SkinningSample** | `SkinningSample_4_0` |
| **Particles3D** | `Particles3DSample_4_0` |
| **BillboardSample** | `BillboardSample_4_0` |
| **InstancedModel** | `InstancedModelSample_4_0` |
| **LensFlare** | `LensFlareSample_4_0` |

### Phase 4 — Full Games

| Sample | XNA Source Dir |
|---|---|
| **MarbleMaze** | `MarbleMaze_4_0` |
| **RolePlayingGame** | `RolePlayingGame_4_0_Win_Xbox` |
| **NetRumble** | `NetRumble_4_0` |
| **HoneycombRush** | `HoneycombRush_4_0` |
| **NinjAcademy** | `NinjAcademy_4_0` |
| **ShipGame** | `ShipGame_4_0` |

### Phase 5 — Audio, AI, Misc

| Sample | XNA Source Dir |
|---|---|
| **Audio3D** | `Audio3DSample_4_0` |
| **SoundAndMusic** | `SoundAndMusic_4_0` |
| **ChaseAndEvade** | `ChaseAndEvadeSample_4_0` |
| **FuzzyLogic** | `FuzzyLogicSample_4_0` |
| **InverseKinematics** | `InverseKinematics_4_0` |
| **WaypointSample** | `WaypointSample_4_0` |
| **SplitScreen** | `SplitScreenSample_4_0` |
| **CameraShake** | `CameraShake_4_0` |

### Excluded / Deferred

The following categories are deferred until CNA gains the corresponding platform support:

- **Phone/Mango-only samples** (`*_PHONE`, `*_Mango`, `PushNotifications`, `Geolocation`, `Accelerometer`) — require WP7 APIs
- **Silverlight samples** (`SilverlightMicrophoneSample`, `PushRecipe_WP7_SL`) — require Silverlight
- **WinForms samples** (`WinFormsGraphicsSample`, `WinFormsContentSample`) — require WinForms
- **Avatar/Rig samples** (`AvatarRig_*`, `AvatarAnimPack_*`) — require Xbox Live Avatar system
- **XNA 2.0 ARCHIVE samples** — older API, lower priority
- **Tool samples** (`BitmapFontMaker`, `CurveEditor`, `XnaGraphicsProfileChecker`) — desktop tooling, not games

---

## File Naming Conventions

- Source files: `PascalCase.cpp` / `PascalCase.hpp` (mirroring original C# filenames)
- CMake targets: `cna_sample_<snake_case_name>` (all lowercase, underscores)
- Directories under `samples/`: `PascalCase` matching the sample name

---

## Coding Conventions

- C++23 standard throughout
- `using namespace Microsoft::Xna::Framework;` is permitted inside `.cpp` files (not headers)
- No `using namespace std;` in headers
- Class fields prefixed with nothing (XNA convention uses camelCase fields, keep it)
- `#pragma once` in all headers
- No copyright header blocks in migrated files (content is credited in NOTICE.md)

---

## Status

| Phase | Status |
|---|---|
| Phase 1 — Foundation | In progress |
| Phase 2 — 2D Games | Planned |
| Phase 3 — 3D Graphics | Planned |
| Phase 4 — Full Games | Planned |
| Phase 5 — Audio, AI, Misc | Planned |

---

## Related Projects

- [CNA](../cna) — C++ XNA 4.0 reimplementation (the framework this samples repo runs on)
- [sharp-runtime](../sharp-runtime) — C++ port of .NET BCL types used by CNA
- [FNA](https://github.com/FNA-XNA/FNA) — Authoritative XNA 4.0 API behavioral reference (local: `/rv/data/library/github.com/FNA-XNA/FNA`)
