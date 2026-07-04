# CNA Samples — Migration Plan

## Overview

This repository contains C++ ports of the official **Microsoft XNA Game Studio 4.0** sample
collection, migrated to run on [CNA](../cna) — a C++ reimplementation of the XNA 4.0
programming model built on SDL3 and a pluggable graphics backend.

The original C# samples are archived at `/rv/tmp/XNAGameStudio/Samples` (local mirror of
[XNA Game Studio archive](https://github.com/SimonDarksideJ/XNAGameStudio)).

For cross-validation, the official MonoGame sample collection is cloned at
`/rv/tmp/MonoGame.Samples`. Of the 83 portable XNA 4.0 samples in this plan,
**only two** have a direct MonoGame equivalent: Platformer (#013) and ShipGame (#066).
Those can be run under MonoGame (DesktopGL target) to compare MonoGame vs. CNA behaviour.

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

Source directory `/rv/tmp/XNAGameStudio/Samples` contains **153 subdirectories**
(excluding `.idea`). All 153 are listed as numbered tasks below.

| Category | Count |
|---|---|
| Phase 1–7 portable desktop samples | 83 |
| Deferred — phone HW / Avatar / WinForms / Xbox Live | 28 |
| XNA 2.0 / 3.0 / 3.1 archive samples | 12 |
| Avatar asset packs (art, no C# code) | 4 |
| Avatar rig packs (art, no C# code) | 3 |
| Phone / Mango platform variants | 7 |
| VB language duplicates | 1 |
| Silverlight / WP7 (no XNA 4.0 code) | 5 |
| Image / resource-only directories | 3 |
| Third-party kits | 3 |
| Unversioned starter kits | 2 |
| Misc / non-code | 2 |
| **Total** | **153** |

---

## Complete Sample Task List

Status legend: ✅ Done · 🔨 In progress · ⬜ Todo · ⚠️ Deferred (CNA gap) · ❌ Out of scope

---

### Phase 1 — Foundation (Core Rendering, Input Basics)

These samples validate the most fundamental CNA APIs and must work before anything else.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 001 | PrimitivesSample | `PrimitivesSample_4_0` | ✅ Done |
| 002 | Primitives3D | `Primitives3DSample_4_0` | ✅ Done |
| 003 | TexturesAndColors | `TexturesAndColorsSample_4_0` | ✅ Done |
| 004 | StockEffects | `StockEffectsSample_4_0` | ❌ Out of scope — effect source + CLI compiler, no runnable Game |
| 005 | ReachGraphicsDemo | `ReachGraphicsDemo_4_0` | ⚠️ Deferred — SpriteFont menus + Model (4 scenes) + SkinnedAnim + custom pipeline types |
| 006 | SpriteEffects | `SpriteEffectsSample_4_0` | ✅ Done |
| 007 | SpriteSheet | `SpriteSheetSample_4_0` | ✅ Done |
| 008 | ShapeRendering | `ShapeRenderingSample_4_0` | ✅ Done |
| 009 | InputReporter | `InputReporter_4_0` | ✅ Done |
| 010 | InputSequence | `InputSequenceSample_4_0` | ✅ Done |
| 011 | SafeArea | `SafeAreaSample_4_0` | ✅ Done |
| 012 | GeneratedGeometry | `GeneratedGeometrySample_4_0` | ✅ Done |

---

### Phase 2 — 2D Games & Gameplay

2D games, collision detection, steering, particle effects, and AI.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 013 | Platformer | `Platformer_4_0` | ✅ Done | MonoGame: `Platformer2D` |
| 014 | Spacewar | `Spacewar_4_0` | 🚫 Deferred — Model + custom shaders + RenderTarget2D + XACT not in CNA |
| 015 | TicTacToe | `TicTacToe_4_0` | ✅ Done |
| 016 | Bounce | `BounceSample_4_0` | ✅ Done |
| 017 | CollisionSample | `CollisionSample_4_0` | ✅ Done |
| 018 | PerPixelCollision | `PerPixelCollisionSample_4_0` | ✅ Done |
| 019 | RectangleCollision | `RectangleCollisionSample_4_0` | ✅ Done |
| 020 | TransformedCollision | `TransformedCollisionSample_4_0` | ✅ Done |
| 021 | PathDrawing | `PathDrawing_4_0` | ✅ Done |
| 022 | Pathfinding | `Pathfinding_4_0` | ✅ Done |
| 023 | WaypointSample | `WaypointSample_4_0` | ✅ Done |
| 024 | FlockingSample | `FlockingSample_4_0` | ✅ Done |
| 025 | ChaseAndEvade | `ChaseAndEvadeSample_4_0` | ✅ Done |
| 026 | AimingSample | `AimingSample_4_0` | ✅ Done |
| 027 | FuzzyLogic | `FuzzyLogicSample_4_0` | ✅ Done |
| 028 | ColorReplacement | `ColorReplacementSample_4_0` | 🚫 Deferred — Model + custom Effect not in CNA |
| 029 | ParticleSample | `ParticleSample_4_0` | ✅ Done |
| 030 | CameraShake | `CameraShake_4_0` | ✅ Done |

---

### Phase 3 — 3D Graphics & Shaders

Post-processing, advanced lighting, shadows, picking, terrain.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 031 | BloomSample | `BloomSample_4_0` | 🚫 Deferred — custom Effect (shaders) + RenderTarget2D + Model not in CNA |
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
| 059 | Audio3D | `Audio3DSample_4_0` | ✅ Done |
| 060 | SoundAndMusic | `SoundAndMusic_4_0` | ✅ Done |

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
| 066 | ShipGame | `ShipGame_4_0` | ⬜ Todo | MonoGame: `ShipGame` |
| 067 | CatapultWars | `CatapultWars_4_0` | ✅ Done |
| 068 | CatapultWarsTrainingKit | `CatapultWarsTrainingKit_4_0` | ⬜ Todo |
| 069 | CardsStarterKit | `CardsStarterKit_4_0` | ⬜ Todo |
| 070 | RolePlayingGame (Win+Xbox) | `RolePlayingGame_4_0_Win_Xbox` | ⬜ Todo |
| 071 | Yacht | `Yacht_4_0` | ✅ Done |
| 072 | GSMSample (Win+Xbox) | `GSMSample_4_0_WIN_XBOX` | ✅ Done | ported as `samples/GameStateManagement` |
| 073 | SoccerPitch | `SoccerPitchSample_4_0` | ⬜ Todo |
| 074 | TankOnHeightmap | `TankOnAHeightMapSample_4_0` | ⚠️ Deferred — same `tank.fbx`/`Tank.cs` per-mesh `ModelBone` lookup as SplitScreen #076, same CNA gap (see `samples/SplitScreen/missing.md`) |

---

### Phase 7 — Advanced, UI, Misc

UI navigation, localization, performance measurement, touch/gesture, networking.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 075 | NGSMSample | `NGSMSample_4_0` | ⚠️ Deferred — Networking/* (~1900 of ~2900 lines) needs `NetworkSession`/`GamerServices`, not in CNA; remaining "Single Player" path is just an empty GameplayScreen stub, not worth porting alone |
| 076 | SplitScreen | `SplitScreenSample_4_0` | ⚠️ Deferred — needs per-mesh `ModelBone` support in CNA's `.model.json` reader (only a single flat Root bone is created today); see `samples/SplitScreen/missing.md` and DEFERRED.md item 6 |
| 077 | DynamicMenu | `DynamicMenu_4_0` | ✅ Done |
| 078 | LocalizationSample | `LocalizationSample_4_0` | ✅ Done |
| 079 | GesturesSample | `GesturesSample_4_0` | ✅ Done |
| 080 | TouchThumbsticks | `TouchThumbsticksSample_4_0` | ✅ Done |
| 081 | PerformanceMeasuring | `PerformanceMeasuringSample_4_0` | ✅ Done |
| 082 | UISample | `UISample_4_0` | ✅ Done |
| 083 | SnowShovel | `SnowShovelSample_4_0` | ✅ Done |

---

### Deferred — Phone Hardware / Avatar / WinForms / Xbox Live Networking

Cannot be ported until CNA gains the relevant platform support, or requires
hardware / services not available on a desktop Linux target.

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
| 092 | ContentManifestExtensions | `ContentManifestExtensions_4_0` | Content pipeline extension, no executable |
| 093 | CurveEditor | `CurveEditor_4_0` | WinForms animation tool |
| 094 | CustomAvatarAnimation | `CustomAvatarAnimation_4_0` | Xbox Live Avatar system |
| 095 | GeolocationSample | `GeolocationSample_4_0` | Phone GPS hardware |
| 096 | InvitesSample | `InvitesSample_4_0` | Xbox Live invite system |
| 097 | MemoryMadnessLab | `MemoryMadnessLab_4_0` | WP7 memory management lab |
| 098 | MicrophoneEcho | `MicrophoneEchoSample_4_0` | Microphone capture hardware |
| 099 | ModelImporterSample | `ModelImporterSample_4_0` | Content pipeline extension, no executable |
| 100 | NetworkPrediction | `NetworkPredictionSample_4_0` | Xbox Live networking stack |
| 101 | ObjectPlacementOnAvatar | `ObjectPlacementOnAvatarSample_4_0` | Xbox Live Avatar system |
| 102 | Orientation | `Orientation_4_0` | Phone orientation sensor |
| 103 | PeerToPeer | `PeerToPeerSample_4_0` | Xbox Live P2P networking |
| 104 | PerformanceUtility | `PerformanceUtility_4_0` | Utility library only, no standalone executable |
| 105 | PushNotifications | `PushNotificationsSample_4_0` | Windows Phone push notifications |
| 106 | SavingEmbeddedImages | `SavingEmbeddedImages_4_0` | Phone media library API |
| 107 | TiltPerspective | `TiltPerspective_4_0` | Phone accelerometer / tilt |
| 108 | WinFormsContent | `WinFormsContentSample_4_0` | WinForms host window |
| 109 | WinFormsGraphics | `WinFormsGraphicsSample_4_0` | WinForms host window |
| 110 | WP7MusicManagement | `WP7MusicManagement_4_0` | Windows Phone 7 media APIs |
| 111 | XnaGraphicsProfileChecker | `XnaGraphicsProfileChecker_4_0` | WinForms diagnostic tool |

---

### XNA 2.0 / 3.0 / 3.1 Archive Samples

Older XNA versions; lower API compatibility with CNA 4.0 target.
Deferred until all _4_0 samples are done and CNA is mature.

| # | Sample Name | Source Directory | XNA Version |
|---|---|---|---|
| 112 | BasicEffectShader | `BasicEffectShader_ARCHIVE_2_0` | XNA 2.0 |
| 113 | Catapult | `Catapult_ARCHIVE_2_0` | XNA 2.0 |
| 114 | MaterialsAndLights | `MaterialsAndLights_ARCHIVE_2_0` | XNA 2.0 |
| 115 | Minjie | `Minjie_ARCHIVE_2_0` | XNA 2.0 |
| 116 | MultipassLighting | `MultipassLighting_ARCHIVE_2_0` | XNA 2.0 |
| 117 | Pickture | `Pickture_ARCHIVE_2_0` | XNA 2.0 |
| 118 | RobotGame | `RobotGame_ARCHIVE_2_0` | XNA 2.0 |
| 119 | SpriteBatchShader | `SpriteBatchShader_ARCHIVE_2_0` | XNA 2.0 |
| 120 | VectorRumble | `VectorRumble_ARCHIVE_2_0` | XNA 2.0 |
| 121 | SpaceShooter | `SpaceShooter_ARCHIVE_3_0` | XNA 3.0 |
| 122 | TiledSprites | `TiledSpritesSample_ARCHIVE_3_1` | XNA 3.1 |
| 123 | RedistributableTTFs | `RedistributableTTFs_ARCHIVE_3_1` | XNA 3.1 (fonts only) |

---

### Avatar Asset Packs (Art, No C# Code)

Animation data for the Xbox Avatar system. No C# game code to port.
Relevant only after Avatar system support is added to CNA.

| # | Name | Source Directory |
|---|---|---|
| 124 | AvatarAnimPack (BIN) | `AvatarAnimPack_4_0_BIN` |
| 125 | AvatarAnimPack (FBX) | `AvatarAnimPack_4_0_FBX` |
| 126 | AvatarAnimPack (Maya) | `AvatarAnimPack_4_0_Maya` |
| 127 | AvatarAnimPack (Mod Tool) | `AvatarAnimPack_4_0_Mod_Tool` |

---

### Avatar Rig Packs (Art, No C# Code)

Rigging data for 3D DCCs. No C# game code to port.

| # | Name | Source Directory |
|---|---|---|
| 128 | AvatarRig (3ds Max 2010) | `AvatarRig_4_0_Max_2010` |
| 129 | AvatarRig (Maya 2009) | `AvatarRig_4_0_Maya_2009` |
| 130 | AvatarRig (SoftImage Mod Tool 7.5) | `AvatarRig_4_0_SoftImage_Mod_Tool7_5` |

---

### Phone / Mango Platform Variants

These are phone-only or Mango-specific variants of samples that already have a
desktop (`_WIN_XBOX`) equivalent listed above (or are phone-exclusive games).

| # | Name | Source Directory | Desktop equivalent |
|---|---|---|---|
| 131 | GSMSample (Mango) | `GSMSample_4_0_Mango` | #072 |
| 132 | GSMSample (Mango VB) | `GSMSample_4_0_Mango_VB` | #072 (VB dup) |
| 133 | GSMSample (Phone) | `GSMSample_4_0_PHONE` | #072 |
| 134 | ModelViewerDemo (Mango) | `ModelViewerDemo_4_0_Mango` | None (phone-only) |
| 135 | PaddleBattle (Mango) | `PaddleBattle_4_0_Mango` | None (phone-only) |
| 136 | PaddleBattle (Mango VB) | `PaddleBattle_4_0_Mango_VB` | None (VB dup) |
| 137 | RolePlayingGame (Phone) | `RolePlayingGame_4_0_Phone` | #070 |

---

### VB Language Duplicates

Visual Basic versions of samples already covered by their C# counterpart above.

| # | Name | Source Directory | C# equivalent |
|---|---|---|---|
| 138 | CardsStarterKit (VB) | `CardsStarterKit_4_0_VB` | #069 |

---

### Silverlight / Windows Phone 7 (No XNA 4.0 Code)

These directories contain Silverlight or WP7-native code, not XNA 4.0 C#.
Not applicable for CNA porting.

| # | Name | Source Directory |
|---|---|---|
| 139 | CustomIndeterminateProgressBar | `CustomIndeterminateProgressBarSample` |
| 140 | NonLinear WP SL Navigation | `NonLinear-WP-SLApp-Navigation-Service` |
| 141 | PushRecipe WP7 | `PushRecipe_WP7_SL` |
| 142 | SilverlightMicrophone | `SilverlightMicrophoneSample` |
| 143 | TombstoningSample | `TombstoningSample` |

---

### Image / Resource-Only Directories

No C# code — contain only image assets used by other samples.

| # | Name | Source Directory |
|---|---|---|
| 144 | ButtonImages | `ButtonImages` |
| 145 | ControllerImages | `ControllerImages` |
| 146 | LobbyChatImages | `LobbyChatImages` |

---

### Third-Party / Community Kits

Not official Microsoft samples. Listed for reference.

| # | Name | Source Directory | Notes |
|---|---|---|---|
| 147 | Riemers Tutorials | `Riemers` | Third-party XNA tutorials |
| 148 | XNA 4 Racing Game Kit | `XNA-4-Racing-Game-Kit-master` | Community game kit |
| 149 | Movipa | `Movipa` | Third-party video playback sample |

---

### Unversioned Starter Kits

No version tag in directory name; likely XNA 3.x or earlier.

| # | Name | Source Directory |
|---|---|---|
| 150 | LevelStarterKit | `LevelStarterKit` |
| 151 | UnitConverterStarterKit | `UnitConverterStarterKit` |

---

### Misc / Non-Code

| # | Name | Source Directory | Notes |
|---|---|---|---|
| 152 | XNA XNB Format | `XNA_XNB_Format` | Binary format documentation, no C# code |
| 153 | SoundLab | `SoundLab` | Audio tool / no XNA 4.0 game code |

---

## Phase Status Overview

| Phase | Samples | Done | Todo | Deferred / Out of scope |
|---|---|---|---|---|
| Phase 1 — Foundation | 12 | 10 | 0 | 2 |
| Phase 2 — 2D Games | 18 | 16 | 0 | 2 |
| Phase 3 — 3D Graphics | 19 | 0 | 19 | 0 |
| Phase 4 — Models & Anim | 9 | 0 | 9 | 0 |
| Phase 5 — Audio | 2 | 2 | 0 | 0 |
| Phase 6 — Full Games | 14 | 3 | 10 | 1 |
| Phase 7 — Advanced / Misc | 9 | 7 | 0 | 2 |
| Deferred (phone/Avatar/WinForms/Live) | 28 | 0 | 0 | 28 |
| XNA 2.0/3.x archives | 12 | 0 | 0 | 12 |
| Avatar asset packs | 4 | 0 | 0 | 4 |
| Avatar rig packs | 3 | 0 | 0 | 3 |
| Phone / Mango variants | 7 | 0 | 0 | 7 |
| VB duplicates | 1 | 0 | 0 | 1 |
| Silverlight / WP7 | 5 | 0 | 0 | 5 |
| Image / resource-only | 3 | 0 | 0 | 3 |
| Third-party | 3 | 0 | 0 | 3 |
| Unversioned starters | 2 | 0 | 0 | 2 |
| Misc / non-code | 2 | 0 | 0 | 2 |
| **Total** | **153** | **38** | **38** | **77** |

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

## MonoGame Cross-Reference

Two MonoGame sample repositories exist for cross-validation.
Run the DesktopGL target on Linux to compare MonoGame vs. CNA behaviour.

### Official MonoGame.Samples (`/rv/tmp/MonoGame.Samples`)

Source: <https://github.com/MonoGame/MonoGame.Samples>

Only 2 of our 83 portable samples have a direct equivalent here:

| CNA task # | XNA source | MonoGame.Samples dir |
|---|---|---|
| 013 | `Platformer_4_0` | `Platformer2D/` |
| 066 | `ShipGame_4_0` | `ShipGame/` |

### CartBlanche/MonoGame-Samples (`/rv/tmp/CartBlanche-MonoGame-Samples`)

Source: <https://github.com/CartBlanche/MonoGame-Samples>  
A community port of ~35 XNA 4.0 samples to MonoGame. Much wider coverage.
Note: XNB assets — not yet converted to open formats in this repo.

| CNA task # | XNA source | CartBlanche dir | Notes |
|---|---|---|---|
| 001 | `PrimitivesSample_4_0` | `Primitives/` | ✓ ported |
| **002** | **`Primitives3DSample_4_0`** | **—** | **NOT ported to MonoGame (3D mesh gen + VertexPositionNormal not done)** |
| 005 | `ReachGraphicsDemo_4_0` | `ReachGraphicsDemo/` | ✓ |
| 006 | `SpriteEffectsSample_4_0` | `SpriteEffects/` | ✓ |
| 009 | `InputReporter_4_0` | `InputReporter/` | ✓ |
| 017 | `CollisionSample_4_0` | `Collisions/` | ✓ (3D BoundingOrientedBox variant) |
| 018 | `PerPixelCollisionSample_4_0` | `PerPixelCollision/` | ✓ |
| 019 | `RectangleCollisionSample_4_0` | `RectangleCollision/` | ✓ |
| 020 | `TransformedCollisionSample_4_0` | `TransformedCollision/` | ✓ |
| 024 | `FlockingSample_4_0` | `Flocking/` | ✓ |
| 025 | `ChaseAndEvadeSample_4_0` | `ChaseAndEvade/` | ✓ |
| 026 | `AimingSample_4_0` | `Aiming/` | ✓ |
| 029 | `ParticleSample_4_0` | `Particle2D/` | ✓ |
| 031 | `BloomSample_4_0` | `BloomEffect/` | ✓ |
| 038 | `ShadowMappingSample_4_0` | `ShadowMapping/` | ✓ |
| 041 | `LensFlareSample_4_0` | `LensFlare/` | ✓ |
| 042 | `ShatterEffectSample_4_0` | `ShatterEffect/` | ✓ |
| 043 | `Particles3DSample_4_0` | `Particle3D/` | ✓ |
| 046 | `Graphics3DSample_4_0` | `Graphics3D/` | ✓ |
| 059 | `Audio3DSample_4_0` | `Audio3D/` | ✓ |
| 060 | `SoundAndMusic_4_0` | `Sound/` | ✓ |
| 062 | `NetRumble_4_0` | `NetRumble/` | ✓ |
| 063 | `HoneycombRush_4_0` | `HoneycombRush/` | ✓ |
| 067 | `CatapultWars_4_0` | `CatapultWars/` | ✓ |
| 069 | `CardsStarterKit_4_0` | `CardsStarterKit/` | ✓ |
| 070 | `RolePlayingGame_4_0_Win_Xbox` | `RolePlayingGame/` | ✓ |
| 072 | `GSMSample_4_0_WIN_XBOX` | `GameStateManagement/` | ✓ |
| 079 | `GesturesSample_4_0` | `TouchGesture/` | ✓ |
| 080 | `TouchThumbsticksSample_4_0` | `VirtualGamePad/` | ✓ |
| 081 | `PerformanceMeasuringSample_4_0` | `PerformanceMeasuring/` | ✓ |
| 023 | `WaypointSample_4_0` | `Waypoints2D/` | ✓ |

Additionally CartBlanche contains samples with no XNA 4.0 equivalent:
`AdMob`, `BouncingBox`, `Colored3DCube`, `FarseerPhysics`, `GameComponents`,
`GooCursor`, `MatchemPoker`, `PacMan`, `RacingGame`, `RenderTarget2D`,
`RockRain`, `Shaders2D`, `Shooter`, `SpriteFont`, `StarWarrior`, `Tetris`,
`TexturedQuad`, `UseCustomVertex`, `VideoPlayer`.

---

## Related Projects

- [CNA](../cna) — C++ XNA 4.0 reimplementation (the framework this samples repo runs on)
- [sharp-runtime](../sharp-runtime) — C++ port of .NET BCL types used by CNA
- [FNA](https://github.com/FNA-XNA/FNA) — Authoritative XNA 4.0 API reference (local: `/rv/data/library/github.com/FNA-XNA/FNA`)
- XNA samples source: `/rv/tmp/XNAGameStudio/Samples`
- MonoGame.Samples: `/rv/tmp/MonoGame.Samples` (<https://github.com/MonoGame/MonoGame.Samples>)
- CartBlanche MonoGame ports: `/rv/tmp/CartBlanche-MonoGame-Samples` (<https://github.com/CartBlanche/MonoGame-Samples>)
