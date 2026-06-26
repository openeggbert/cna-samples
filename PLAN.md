# CNA Samples — Migration Plan

## Overview

This repository contains C++ ports of the official **Microsoft XNA Game Studio 4.0** sample
collection, migrated to run on [CNA](../cna) — a C++ reimplementation of the XNA 4.0
programming model built on SDL3 and a pluggable graphics backend.

The original C# samples are archived at `/rv/tmp/XNAGameStudio/Samples` (local mirror of
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
├── CMakeLists.txt             # Root build; finds CNA, registers each sample subdirectory
├── CMakePresets.json          # Convenience presets (debug, release)
├── cmake/
│   └── SampleHelpers.cmake    # cna_add_sample() helper macro
├── samples/
│   └── SampleName/
│       ├── CMakeLists.txt     # calls cna_add_sample()
│       └── src/
│           ├── Program.cpp    # int main(); includes CNA/Entrypoint.hpp
│           └── *.hpp          # game and helper classes (inline/header-only per sample)
├── .gitignore
├── LICENSE                    # Microsoft Permissive License (Ms-PL) — matches XNA originals
├── NOTICE.md
├── PLAN.md                    # This file
├── DEFERRED.md                # CNA feature gaps blocking specific ports
├── CLAUDE.md                  # Porting guidelines for Claude Code sessions
└── README.md
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

CMake cache variables forwarded to CNA:

| Variable | Value |
|---|---|
| `CNA_BUILD_TESTS` | `OFF` |
| `CNA_BUILD_EXAMPLES` | `OFF` |
| `SHARP_RUNTIME_BUILD_TESTS` | `OFF` |

### CMake Targets per Sample

Each sample `SampleName` produces a single executable target named `cna_sample_<lowercase_name>`.

Example: `cna_sample_primitives`, `cna_sample_primitives3d`.

### Linking Pattern

Handled automatically by `cna_add_sample()` in `cmake/SampleHelpers.cmake`:

```cmake
target_link_libraries(target PRIVATE
    -Wl,--start-group CNA ${BACKEND_TARGET} -Wl,--end-group SHARP_RUNTIME)
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
| `foreach (var x in list)` | range-for `for (auto& x : list)` |
| `Math.Sin(x)` | `std::sin(x)` |
| `Random` | `std::mt19937` / `std::uniform_real_distribution` |
| `TimeSpan` | `sharp_runtime::TimeSpan` (from sharp-runtime) |
| `int?` / nullable | `std::optional<int>` |
| Properties (`get; set;`) | `getXxxProperty()` / `setXxxProperty()` via `DEF_PROP` |

### Content / Assets

XNA uses compiled `.xnb` binary assets — **not supported by CNA**.
Use raw formats instead: PNG textures, OGG/WAV audio, glTF models.
Each sample's `Content/` directory is copied next to the built executable.

---

## Sample Count Summary

| Category | Count |
|---|---|
| Primary `_4_0` samples (code) | 109 |
| Desktop platform variants included | 2 |
| **Total tasks in this plan** | **111** |
| Phases 1–7 (portable, planned to port) | 83 |
| Deferred (phone HW / Avatar / WinForms / Xbox Live) | 28 |

**Not counted** (non-C# code, duplicates, archives):
art asset packs (`AvatarAnimPack_4_0_*`, `AvatarRig_4_0_*`),
VB duplicates (`CardsStarterKit_4_0_VB`, `GSMSample_4_0_Mango_VB`, `PaddleBattle_4_0_Mango_VB`),
phone-only variants (`GSMSample_4_0_PHONE`, `GSMSample_4_0_Mango`, `PaddleBattle_4_0_Mango`,
`ModelViewerDemo_4_0_Mango`, `RolePlayingGame_4_0_Phone`),
XNA 2.0/3.x ARCHIVE items, Silverlight samples, image-only dirs,
third-party kits, `.msi` installers.

---

## Complete Sample Task List

Status legend: ✅ Done · 🔨 In progress · ⬜ Todo · ⚠️ Deferred

---

### Phase 1 — Foundation (Core Rendering, Input Basics)

These samples validate the most fundamental CNA APIs and must work before anything else.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 001 | PrimitivesSample | `PrimitivesSample_4_0` | ✅ Done |
| 002 | Primitives3D | `Primitives3DSample_4_0` | ✅ Done |
| 003 | TexturesAndColors | `TexturesAndColorsSample_4_0` | ⬜ Todo |
| 004 | StockEffects | `StockEffectsSample_4_0` | ⬜ Todo |
| 005 | ReachGraphicsDemo | `ReachGraphicsDemo_4_0` | ⬜ Todo |
| 006 | SpriteEffects | `SpriteEffectsSample_4_0` | ⬜ Todo |
| 007 | SpriteSheet | `SpriteSheetSample_4_0` | ⬜ Todo |
| 008 | ShapeRendering | `ShapeRenderingSample_4_0` | ⬜ Todo |
| 009 | InputReporter | `InputReporter_4_0` | ⬜ Todo |
| 010 | InputSequence | `InputSequenceSample_4_0` | ⬜ Todo |
| 011 | SafeArea | `SafeAreaSample_4_0` | ⬜ Todo |
| 012 | GeneratedGeometry | `GeneratedGeometrySample_4_0` | ⬜ Todo |

---

### Phase 2 — 2D Games & Gameplay

2D games, collision detection, steering, particle effects, and AI.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 013 | Platformer | `Platformer_4_0` | ⬜ Todo |
| 014 | Spacewar | `Spacewar_4_0` | ⬜ Todo |
| 015 | TicTacToe | `TicTacToe_4_0` | ⬜ Todo |
| 016 | Bounce | `BounceSample_4_0` | ⬜ Todo |
| 017 | CollisionSample | `CollisionSample_4_0` | ⬜ Todo |
| 018 | PerPixelCollision | `PerPixelCollisionSample_4_0` | ⬜ Todo |
| 019 | RectangleCollision | `RectangleCollisionSample_4_0` | ⬜ Todo |
| 020 | TransformedCollision | `TransformedCollisionSample_4_0` | ⬜ Todo |
| 021 | PathDrawing | `PathDrawing_4_0` | ⬜ Todo |
| 022 | Pathfinding | `Pathfinding_4_0` | ⬜ Todo |
| 023 | WaypointSample | `WaypointSample_4_0` | ⬜ Todo |
| 024 | FlockingSample | `FlockingSample_4_0` | ⬜ Todo |
| 025 | ChaseAndEvade | `ChaseAndEvadeSample_4_0` | ⬜ Todo |
| 026 | AimingSample | `AimingSample_4_0` | ⬜ Todo |
| 027 | FuzzyLogic | `FuzzyLogicSample_4_0` | ⬜ Todo |
| 028 | ColorReplacement | `ColorReplacementSample_4_0` | ⬜ Todo |
| 029 | ParticleSample | `ParticleSample_4_0` | ⬜ Todo |
| 030 | CameraShake | `CameraShake_4_0` | ⬜ Todo |

---

### Phase 3 — 3D Graphics & Shaders

Post-processing, advanced lighting, shadows, picking, terrain.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 031 | BloomSample | `BloomSample_4_0` | ⬜ Todo |
| 032 | DistortionSample | `DistortionSample_4_0` | ⬜ Todo |
| 033 | NonPhotoRealistic | `NonPhotoRealisticSample_4_0` | ⬜ Todo |
| 034 | NormalMapping | `NormalMappingSample_4_0` | ⬜ Todo |
| 035 | PerPixelLighting | `PerPixelLightingSample_4_0` | ⬜ Todo |
| 036 | VertexLighting | `VertexLightingSample_4_0` | ⬜ Todo |
| 037 | RimLighting | `RimLighting_4_0` | ⬜ Todo |
| 038 | ShadowMapping | `ShadowMappingSample_4_0` | ⬜ Todo |
| 039 | BillboardSample | `BillboardSample_4_0` | ⬜ Todo |
| 040 | InstancedModel | `InstancedModelSample_4_0` | ⬜ Todo |
| 041 | LensFlare | `LensFlareSample_4_0` | ⬜ Todo |
| 042 | ShatterEffect | `ShatterEffectSample_4_0` | ⬜ Todo |
| 043 | Particles3D | `Particles3DSample_4_0` | ⬜ Todo |
| 044 | Particles2DPipeline | `Particles2DPipeline_4_0` | ⬜ Todo |
| 045 | XmlParticles | `XmlParticles_4_0` | ⬜ Todo |
| 046 | Graphics3D | `Graphics3DSample_4_0` | ⬜ Todo |
| 047 | PickingSample | `PickingSample_4_0` | ⬜ Todo |
| 048 | TrianglePicking | `TrianglePickingSample_4_0` | ⬜ Todo |
| 049 | HeightmapCollision | `HeightmapCollisionSample_4_0` | ⬜ Todo |

---

### Phase 4 — Models & Animation

3D model loading, skeletal animation, inverse kinematics, camera systems.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 050 | SimpleAnimation | `SimpleAnimation_4_0` | ⬜ Todo |
| 051 | CustomModelAnimation | `CustomModelAnimation_4_0` | ⬜ Todo |
| 052 | CustomModelClass | `CustomModelClassSample_4_0` | ⬜ Todo |
| 053 | CustomModelEffect | `CustomModelEffectSample_4_0` | ⬜ Todo |
| 054 | SkinningSample | `SkinningSample_4_0` | ⬜ Todo |
| 055 | SkinnedModelExtensions | `SkinnedModelExtensions_4_0` | ⬜ Todo |
| 056 | CPUSkinning | `CPUSkinningSample_4_0` | ⬜ Todo |
| 057 | InverseKinematics | `InverseKinematics_4_0` | ⬜ Todo |
| 058 | ChaseCamera | `ChaseCamera_4_0` | ⬜ Todo |

---

### Phase 5 — Audio

3D positional audio, background music, sound effects.
Requires CNA audio backend (see `DEFERRED.md` item 7).

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 059 | Audio3D | `Audio3DSample_4_0` | ⬜ Todo |
| 060 | SoundAndMusic | `SoundAndMusic_4_0` | ⬜ Todo |

---

### Phase 6 — Full Games & Starter Kits

Complete games and starter kits — the most demanding ports.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 061 | MarbleMaze | `MarbleMaze_4_0` | ⬜ Todo |
| 062 | NetRumble | `NetRumble_4_0` | ⬜ Todo |
| 063 | HoneycombRush | `HoneycombRush_4_0` | ⬜ Todo |
| 064 | HoneycombRushTrainingKit | `HoneycombRushTrainingKit_4_0` | ⬜ Todo |
| 065 | NinjAcademy | `NinjAcademy_4_0` | ⬜ Todo |
| 066 | ShipGame | `ShipGame_4_0` | ⬜ Todo |
| 067 | CatapultWars | `CatapultWars_4_0` | ⬜ Todo |
| 068 | CatapultWarsTrainingKit | `CatapultWarsTrainingKit_4_0` | ⬜ Todo |
| 069 | CardsStarterKit | `CardsStarterKit_4_0` | ⬜ Todo |
| 070 | RolePlayingGame | `RolePlayingGame_4_0_Win_Xbox` | ⬜ Todo |
| 071 | Yacht | `Yacht_4_0` | ⬜ Todo |
| 072 | GSMSample | `GSMSample_4_0_WIN_XBOX` | ⬜ Todo |
| 073 | SoccerPitch | `SoccerPitchSample_4_0` | ⬜ Todo |
| 074 | TankOnHeightmap | `TankOnAHeightMapSample_4_0` | ⬜ Todo |

---

### Phase 7 — Advanced, UI, Misc

UI navigation, localization, performance measurement, touch/gesture, networking.

| # | Sample Name | Source Directory | Notes |
|---|---|---|---|
| 075 | NGSMSample | `NGSMSample_4_0` | Network-aware Game State Management |
| 076 | SplitScreen | `SplitScreenSample_4_0` | Multiple viewport split screen |
| 077 | DynamicMenu | `DynamicMenu_4_0` | Runtime-built UI menu |
| 078 | LocalizationSample | `LocalizationSample_4_0` | String/resource localization |
| 079 | GesturesSample | `GesturesSample_4_0` | Touch/mouse gesture recognition |
| 080 | TouchThumbsticks | `TouchThumbsticksSample_4_0` | Virtual on-screen thumbstick |
| 081 | PerformanceMeasuring | `PerformanceMeasuringSample_4_0` | Frame timing & profiling |
| 082 | UISample | `UISample_4_0` | Menu/UI navigation system |
| 083 | SnowShovel | `SnowShovelSample_4_0` | Physics-based 2D game |

All Phase 7 samples: status ⬜ Todo

---

### Deferred — Platform-specific, Tools, Xbox Live

These samples cannot be ported until CNA gains the relevant platform support,
or require hardware not available on a desktop Linux target.
They are listed here for completeness.

| # | Sample Name | Source Directory | Reason deferred |
|---|---|---|---|
| 084 | AccelerometerSample | `AccelerometerSample_4_0` | Phone accelerometer hardware |
| 085 | AvatarAnimationBlending | `AvatarAnimationBlendingSample_4_0` | Xbox Live Avatar system |
| 086 | AvatarMultipleAnimations | `AvatarMultipleAnimationsSample_4_0` | Xbox Live Avatar system |
| 087 | AvatarShadows | `AvatarShadows_4_0` | Xbox Live Avatar system |
| 088 | BingMaps | `BingMaps_4_0` | Bing Maps REST API |
| 089 | BingMapsPathFinding | `BingMapsPathFinding_4_0` | Bing Maps REST API |
| 090 | BitmapFontMaker | `BitmapFontMaker_4_0` | WinForms desktop tool |
| 091 | ClientServerSample | `ClientServerSample_4_0` | Xbox Live networking stack |
| 092 | ContentManifestExtensions | `ContentManifestExtensions_4_0` | Content pipeline extension (not a game) |
| 093 | CurveEditor | `CurveEditor_4_0` | WinForms animation tool |
| 094 | CustomAvatarAnimation | `CustomAvatarAnimation_4_0` | Xbox Live Avatar system |
| 095 | GeolocationSample | `GeolocationSample_4_0` | Phone GPS hardware |
| 096 | InvitesSample | `InvitesSample_4_0` | Xbox Live invite system |
| 097 | MemoryMadnessLab | `MemoryMadnessLab_4_0` | WP7 memory management lab |
| 098 | MicrophoneEcho | `MicrophoneEchoSample_4_0` | Microphone capture hardware |
| 099 | ModelImporterSample | `ModelImporterSample_4_0` | Content pipeline extension (not a game) |
| 100 | NetworkPrediction | `NetworkPredictionSample_4_0` | Xbox Live networking stack |
| 101 | ObjectPlacementOnAvatar | `ObjectPlacementOnAvatarSample_4_0` | Xbox Live Avatar system |
| 102 | Orientation | `Orientation_4_0` | Phone orientation sensor |
| 103 | PeerToPeer | `PeerToPeerSample_4_0` | Xbox Live P2P networking |
| 104 | PerformanceUtility | `PerformanceUtility_4_0` | Utility library only (no standalone executable) |
| 105 | PushNotifications | `PushNotificationsSample_4_0` | Windows Phone push notifications |
| 106 | SavingEmbeddedImages | `SavingEmbeddedImages_4_0` | Phone media library API |
| 107 | TiltPerspective | `TiltPerspective_4_0` | Phone accelerometer / tilt |
| 108 | WinFormsContent | `WinFormsContentSample_4_0` | WinForms host window |
| 109 | WinFormsGraphics | `WinFormsGraphicsSample_4_0` | WinForms host window |
| 110 | WP7MusicManagement | `WP7MusicManagement_4_0` | Windows Phone 7 media APIs |
| 111 | XnaGraphicsProfileChecker | `XnaGraphicsProfileChecker_4_0` | WinForms diagnostic tool |

---

## Phase Status Overview

| Phase | Samples | Done | Todo | Deferred |
|---|---|---|---|---|
| Phase 1 — Foundation | 12 | 2 | 10 | 0 |
| Phase 2 — 2D Games | 18 | 0 | 18 | 0 |
| Phase 3 — 3D Graphics | 19 | 0 | 19 | 0 |
| Phase 4 — Models & Anim | 9 | 0 | 9 | 0 |
| Phase 5 — Audio | 2 | 0 | 2 | 0 |
| Phase 6 — Full Games | 14 | 0 | 14 | 0 |
| Phase 7 — Advanced / Misc | 9 | 0 | 9 | 0 |
| Deferred | 28 | 0 | 0 | 28 |
| **Total** | **111** | **2** | **81** | **28** |

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
- `#pragma once` in all headers
- No copyright header blocks in migrated files (content is credited in `NOTICE.md`)

---

## Related Projects

- [CNA](../cna) — C++ XNA 4.0 reimplementation (the framework this samples repo runs on)
- [sharp-runtime](../sharp-runtime) — C++ port of .NET BCL types used by CNA
- [FNA](https://github.com/FNA-XNA/FNA) — Authoritative XNA 4.0 API reference (local: `/rv/data/library/github.com/FNA-XNA/FNA`)
- XNA samples source: `/rv/tmp/XNAGameStudio/Samples`
