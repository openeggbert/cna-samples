# CNA Samples — Migration Plan

## Overview

This repository contains C++ ports of the official **Microsoft XNA Game Studio 4.0** sample
collection, migrated to run on [CNA](../cna) — a C++ reimplementation of the XNA 4.0
programming model built on SDL3 and a pluggable graphics backend.

The original C# samples are archived at `/rv/tmp/XNAGameStudio/Samples` (local mirror of
[XNA Game Studio archive](https://github.com/SimonDarksideJ/XNAGameStudio)).

Every sample this repo tracks — done, in progress, or blocked on a CNA gap — gets a
`samples/<Name>/` directory containing at minimum `<Name>.htm` (the original doc,
copied verbatim) and `missing.md` (what's missing vs. the CNA port, or why it isn't
ported yet). Samples that will **never** get a directory here (not real XNA 4.0
games, WinForms/content-pipeline-only tools, redundant training-kit duplicates, or
tied to a platform/service CNA won't ever provide) are listed instead in
[`ignored.md`](ignored.md), one line per sample with its reason.

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
(excluding `.idea`). All 153 are accounted for below: either a numbered task in
this file (with a `samples/<Name>/` directory — done, or a placeholder) or a line
in `ignored.md` (never gets a directory).

| Category | Count | Where |
|---|---|---|
| Done (real port, builds) | 63 | this file |
| 🔓 Unblocked (placeholder exists, CNA gap since resolved — ready to port) | 0 | this file |
| 🚧 Placeholder (still blocked on a CNA gap or scope decision) | 23 | this file |
| ❌ Ignored — never gets a directory | 67 | `ignored.md` |
| **Total** | **153** | |

**Update 2026-07-11 (SimpleAnimation #050):** status line corrected from stale
🚧 Placeholder to ✅ Done — the sample has had real source since Tasks 936/937 (per-mesh
`ModelBone` support); two real CNA bugs (tank-mesh winding reversal, `GraphicsDevice`
default-state depth-occlusion gap) were found and fixed against it this session — see
`samples/SimpleAnimation/missing.md` and `../cna_graphics/plan_graphics.md` Phase 79 (Task 1006)
for the full account and remaining future-review items. **A parallel, repo-wide re-audit of all
153 catalogued samples — including the 67 in `ignored.md` — is now tracked one-task-per-sample in
`../cna_graphics/plan_graphics.md`'s new Phase 79**, prompted directly by this discovery (a sample
marked "Done" here can still hide a real, unfound CNA-level bug). Treat Phase 79 as the current
authoritative per-sample re-verification tracker; this file's own per-sample status lines will be
corrected here as each Phase 79 task closes.

**Update 2026-07-10 (RimLighting #037):** ported via the same
construct-the-real-C++-object-directly bypass ReachGraphicsDemo's `EnvmapDemo`
established for its own cubemap — `Content.Load<TextureCube>` (item #14) and
`Content.Load<Model>` (item #26) were both bypassed, unblocking what
`NEXT.md`/`DEFERRED.md` had flagged as "the closest remaining portable sample."
Found and fixed a real `tools/fbx_ascii2model.py` bug along the way (assumed
`LayerElementNormal` is always `"ByPolygonVertex"`; it's usually `"ByVertice"` —
DEFERRED.md item #30, new) that also likely affects ChaseCamera's `Ship.fbx` and
ReachGraphicsDemo's `saucer.fbx`/`model.fbx` (not re-verified/re-shipped this
session). See `samples/RimLighting/missing.md` for the full account.

**Update 2026-07-10:** all 10 previously-🔓-unblocked samples were ported this
session (LensFlare #041, Graphics3D #046, PickingSample #047, TrianglePicking
#048, HeightmapCollision #049, InverseKinematics #057, ChaseCamera #058,
MarbleMaze #061, NetworkPrediction #100, PeerToPeer #103), plus
AccelerometerSample (#084, user-approved keyboard-tilt fallback — the
original's own emulator-testing code, ported nearly verbatim) and
TiltPerspective (#107, same user approval, an invented keyboard-tilt scheme
since that original's own fallback auto-wobbles instead). See `NEXT.md`
section 3 for the full account of each, including a major new CNA bug found
along the way (DEFERRED.md item #26 — a vertex-stride/vtable mismatch in
`ModelTypeReader::Read()` now believed to be the true cause of the
long-tracked "near-plane-clipping" rendering symptom, confirmed on 5+ assets
across 5 samples) and two build-breakage fixes (`SafeArea`,
`RolePlayingGame` — stale `Viewport.x`/`.y` direct-field access). The 5
Avatar samples (#085/#086/#087/#094/#101), previously listed as 🔓-reopened
via `cna`'s substitute-body rendering path, were **permanently declined**
(user go/no-go, 2026-07-10 — the substitute visual isn't faithful enough to
be worth porting) and moved back to `ignored.md`.

**Update (2026-07-10, later the same session): `ReachGraphicsDemo` (#005) has
now been ported**, confirming the stale-`missing.md` finding above. 5 of its 6
demo scenes (`BasicDemo`/`AlphaDemo`/`DualDemo`/`EnvmapDemo`/`ParticleDemo`,
plus the shared menu framework) are done; `SkinnedDemo` was skipped (still
genuinely blocked on DEFERRED item #13, skeletal animation — replaced with a
clear in-app "not available" message). Found and worked around 2 new CNA
rendering gaps in the process (DEFERRED.md items #28/#29) and fixed a real
bug in `tools/fbx_ascii2model.py` (a multi-UV-layer FBX mesh parsing bug). See
`samples/ReachGraphicsDemo/missing.md` and `NEXT.md` section 3 for the full
account.

Of the 41 placeholder-or-unblocked directories originally identified in an
earlier sweep: 5 predate that sweep (ReachGraphicsDemo, Spacewar,
ColorReplacement, BloomSample, SplitScreen) and 36 were added in one pass
across Phase 3, Phase 4, the remaining Phase 6 samples, and 6 of the 27
"Deferred — Phone Hardware" appendix samples. Of those 36, **13 turned out to
already be unblocked** when their cited `DEFERRED.md` gap was actually checked
against `cna`'s live source on 2026-07-06 (see `NEXT.md` section 3 for the
full story) — all 13 are now ✅ Done (see the update note above). NetRumble
(#062) went from double- to single-blocked (item 11 remains). See each
`missing.md` for the specific `DEFERRED.md` item.

---

## Complete Sample Task List

Status legend: ✅ Done · 🔨 In progress · ⬜ Todo (no directory yet) ·
🚧 Placeholder (`samples/<Name>/<Name>.htm` + `missing.md` exist, blocked on a CNA
gap — see `DEFERRED.md`) · 🔓 Unblocked (placeholder exists but its CNA gap has
since been resolved — ready to port, no CNA change needed) ·
❌ Ignored (never gets a directory — see `ignored.md`)

---

### Phase 1 — Foundation (Core Rendering, Input Basics)

These samples validate the most fundamental CNA APIs and must work before anything else.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 001 | PrimitivesSample | `PrimitivesSample_4_0` | ✅ Done |
| 002 | Primitives3D | `Primitives3DSample_4_0` | ✅ Done |
| 003 | TexturesAndColors | `TexturesAndColorsSample_4_0` | ✅ Done |
| 004 | StockEffects | `StockEffectsSample_4_0` | ❌ Out of scope — effect source + CLI compiler, no runnable Game |
| 005 | ReachGraphicsDemo | `ReachGraphicsDemo_4_0` | ✅ Done (ported 2026-07-10; 5 of 6 demo scenes — `SkinnedDemo` skipped, DEFERRED item #13; found 2 new CNA rendering gaps, DEFERRED items #28/#29, and fixed a real `tools/fbx_ascii2model.py` multi-UV-layer bug — see `samples/ReachGraphicsDemo/missing.md`) |
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
| 028 | ColorReplacement | `ColorReplacementSample_4_0` | 🚫 Deferred — custom `ReplaceColor.fx` shader not in CNA (item #11); model conversion itself is not a blocker (item #6 resolved) — see `samples/ColorReplacement/missing.md` |
| 029 | ParticleSample | `ParticleSample_4_0` | ✅ Done |
| 030 | CameraShake | `CameraShake_4_0` | ✅ Done |

---

### Phase 3 — 3D Graphics & Shaders

Post-processing, advanced lighting, shadows, picking, terrain.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 031 | BloomSample | `BloomSample_4_0` | 🚫 Deferred — custom Effect (shaders) not in CNA (RenderTarget2D itself is ✅ resolved, DEFERRED.md item 12; the 3 custom .fx files — GaussianBlur/BloomCombine/BloomExtract — are the remaining blocker) |
| 032 | DistortionSample | `DistortionSample_4_0` | 🚧 Placeholder — see `samples/DistortionSample/missing.md` |
| 033 | NonPhotoRealistic | `NonPhotoRealisticSample_4_0` | 🚧 Placeholder — see `samples/NonPhotoRealistic/missing.md` |
| 034 | NormalMapping | `NormalMappingSample_4_0` | 🚧 Placeholder — see `samples/NormalMapping/missing.md` |
| 035 | PerPixelLighting | `PerPixelLightingSample_4_0` | 🚧 Placeholder — see `samples/PerPixelLighting/missing.md` |
| 036 | VertexLighting | `VertexLightingSample_4_0` | 🚧 Placeholder — see `samples/VertexLighting/missing.md` |
| 037 | RimLighting | `RimLighting_4_0` | ✅ Done (ported 2026-07-10; `EnvironmentMapEffect`-based rim lighting against a real 6-face `OutputCube.dds`, bypassing both `Content.Load<TextureCube>` (item #14) and `Content.Load<Model>` (item #26); found and fixed a real `tools/fbx_ascii2model.py` normal-mapping bug — see `samples/RimLighting/missing.md`, DEFERRED item #30) |
| 038 | ShadowMapping | `ShadowMappingSample_4_0` | 🚧 Placeholder — see `samples/ShadowMapping/missing.md` |
| 039 | BillboardSample | `BillboardSample_4_0` | 🚧 Placeholder — see `samples/BillboardSample/missing.md` |
| 040 | InstancedModel | `InstancedModelSample_4_0` | 🚧 Placeholder — see `samples/InstancedModel/missing.md` |
| 041 | LensFlare | `LensFlareSample_4_0` | ✅ Done (ported 2026-07-09/10; confirms the near-plane-clipping/DEFERRED item #26 symptom on a second independent asset; found a real `fbx_ascii2model.py` node-transform bug along the way — see `samples/LensFlare/missing.md`) |
| 042 | ShatterEffect | `ShatterEffectSample_4_0` | 🚧 Placeholder — see `samples/ShatterEffect/missing.md` |
| 043 | Particles3D | `Particles3DSample_4_0` | 🚧 Placeholder — see `samples/Particles3D/missing.md` |
| 044 | Particles2DPipeline | `Particles2DPipeline_4_0` | ✅ Done |
| 045 | XmlParticles | `XmlParticles_4_0` | 🚧 Placeholder — see `samples/XmlParticles/missing.md` |
| 046 | Graphics3D | `Graphics3DSample_4_0` | ✅ Done (ported 2026-07-10; found and worked around a real CNA component-lifecycle gap, DEFERRED item #23 — see `samples/Graphics3D/missing.md`) |
| 047 | PickingSample | `PickingSample_4_0` | ✅ Done (ported 2026-07-10; found the "flat white untextured model" rendering pattern later confirmed on several sibling samples — see `samples/PickingSample/missing.md`) |
| 048 | TrianglePicking | `TrianglePickingSample_4_0` | ✅ Done (ported 2026-07-10; real per-triangle Möller–Trumbore picking, extended `tools/fbx_ascii2model.py` with a `--picking` sidecar since CNA's `VertexBuffer`/`IndexBuffer` have no `GetData()`, DEFERRED item #25 — see `samples/TrianglePicking/missing.md`) |
| 049 | HeightmapCollision | `HeightmapCollisionSample_4_0` | ✅ Done (ported 2026-07-10; hand-built runtime terrain mesh — the first sample with a real bound texture, sidestepping the `.model.json` texture-field gap — see `samples/HeightmapCollision/missing.md`) |

---

### Phase 4 — Models & Animation

3D model loading, skeletal animation, inverse kinematics, camera systems.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 050 | SimpleAnimation | `SimpleAnimation_4_0` | ✅ Done (status corrected 2026-07-11 — this row was stale; the sample has had real, working source since the multi-bone `ModelBone` fix, Tasks 936/937. Two real CNA bugs found and fixed against it 2026-07-11: a systematic tank-mesh winding reversal (`../cna_graphics` Task 954) and a `GraphicsDevice` default-state depth-occlusion bug, Task 955. See `samples/SimpleAnimation/missing.md` for the full account. Flagged for a future pixel-perfect re-review — see `../cna_graphics/plan_graphics.md` Phase 79, Task 1006.) |
| 051 | CustomModelAnimation | `CustomModelAnimation_4_0` | 🚧 Placeholder — see `samples/CustomModelAnimation/missing.md` |
| 052 | CustomModelClass | `CustomModelClassSample_4_0` | ✅ Done (ported 2026-07-06; renders correctly except the pre-existing CameraShake near-plane clipping bug, same asset) |
| 053 | CustomModelEffect | `CustomModelEffectSample_4_0` | 🚧 Placeholder — see `samples/CustomModelEffect/missing.md` |
| 054 | SkinningSample | `SkinningSample_4_0` | 🚧 Placeholder — see `samples/SkinningSample/missing.md` |
| 055 | SkinnedModelExtensions | `SkinnedModelExtensions_4_0` | 🚧 Placeholder — see `samples/SkinnedModelExtensions/missing.md` |
| 056 | CPUSkinning | `CPUSkinningSample_4_0` | 🚧 Placeholder — see `samples/CPUSkinning/missing.md` |
| 057 | InverseKinematics | `InverseKinematics_4_0` | ✅ Done (ported 2026-07-10; found DEFERRED item #26 — a `ModelTypeReader` vertex-stride/vtable corruption bug now believed to be the true cause of the "near-plane-clipping" symptom family — see `samples/InverseKinematics/missing.md`) |
| 058 | ChaseCamera | `ChaseCamera_4_0` | ✅ Done (ported 2026-07-10; reconfirmed DEFERRED item #26 on 2 more assets at both size extremes — see `samples/ChaseCamera/missing.md`) |

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
| 061 | MarbleMaze | `MarbleMaze_4_0` | ✅ Done (ported 2026-07-10, `Source/EX2_Polishing/End/` — a full game: menu, physics, tilt maze, HUD; second confirmed `assimp`-export winding-inversion quirk — see `samples/MarbleMaze/missing.md`) |
| 062 | NetRumble | `NetRumble_4_0` | 🚧 Placeholder — networking unblocked 2026-07-06, still needs DEFERRED.md item 11 (shaders); see `samples/NetRumble/missing.md` |
| 063 | HoneycombRush | `HoneycombRush_4_0` | ✅ Done |
| 064 | HoneycombRushTrainingKit | `HoneycombRushTrainingKit_4_0` | ❌ Ignored — see `ignored.md` |
| 065 | NinjAcademy | `NinjAcademy_4_0` | ✅ Done |
| 066 | ShipGame | `ShipGame_4_0` | 🚧 Placeholder — see `samples/ShipGame/missing.md` | MonoGame: `ShipGame` |
| 067 | CatapultWars | `CatapultWars_4_0` | ✅ Done |
| 068 | CatapultWarsTrainingKit | `CatapultWarsTrainingKit_4_0` | ❌ Ignored — see `ignored.md` |
| 069 | CardsStarterKit | `CardsStarterKit_4_0` | ✅ Done |
| 070 | RolePlayingGame (Win+Xbox) | `RolePlayingGame_4_0_Win_Xbox` | ✅ Done | Combat/several screens simplified — see `samples/RolePlayingGame/missing.md` |
| 071 | Yacht | `Yacht_4_0` | ✅ Done |
| 072 | GSMSample (Win+Xbox) | `GSMSample_4_0_WIN_XBOX` | ✅ Done | ported as `samples/GameStateManagement` |
| 073 | SoccerPitch | `SoccerPitchSample_4_0` | ✅ Done |
| 074 | TankOnHeightmap | `TankOnAHeightMapSample_4_0` | 🚧 Placeholder — see `samples/TankOnHeightmap/missing.md` (same `tank.fbx`/`Tank.cs` per-mesh `ModelBone` gap as SplitScreen #076) |

---

### Phase 7 — Advanced, UI, Misc

UI navigation, localization, performance measurement, touch/gesture, networking.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 075 | NGSMSample | `NGSMSample_4_0` | ❌ Ignored — see `ignored.md` (would only ever demonstrate an empty stub, even with networking) |
| 076 | SplitScreen | `SplitScreenSample_4_0` | 🚧 Placeholder — needs per-mesh `ModelBone` support in CNA's `.model.json` reader (only a single flat Root bone is created today); see `samples/SplitScreen/missing.md` and DEFERRED.md item 6 |
| 077 | DynamicMenu | `DynamicMenu_4_0` | ✅ Done |
| 078 | LocalizationSample | `LocalizationSample_4_0` | ✅ Done |
| 079 | GesturesSample | `GesturesSample_4_0` | ✅ Done |
| 080 | TouchThumbsticks | `TouchThumbsticksSample_4_0` | ✅ Done |
| 081 | PerformanceMeasuring | `PerformanceMeasuringSample_4_0` | ✅ Done |
| 082 | UISample | `UISample_4_0` | ✅ Done |
| 083 | SnowShovel | `SnowShovelSample_4_0` | ✅ Done |
| 102 | Orientation | `Orientation_4_0` | ✅ Done — moved here from the "Deferred — Phone Hardware" table below; was miscategorized, had zero accelerometer/sensor dependency (see `samples/Orientation/missing.md`) |

---

### Deferred — Phone Hardware / Avatar / WinForms / Xbox Live Networking

Of the 27 `_4_0` samples in this appendix, only 6 make sense as future candidates
(kept below, each now with a placeholder directory); the other 21 are permanently
excluded — see `ignored.md` for the reason each one will never get a directory here.

| # | Sample Name | Source Directory | Status |
|---|---|---|---|
| 084 | AccelerometerSample | `AccelerometerSample_4_0` | ✅ Done (ported 2026-07-10, user-approved go/no-go; the keyboard-tilt fallback turned out to be the original's own emulator-testing code, nested inside `#if WINDOWS_PHONE` — ported nearly verbatim, not invented — see `samples/AccelerometerSample/missing.md`) |
| 085 | AvatarAnimationBlending | `AvatarAnimationBlendingSample_4_0` | ❌ Ignored — see `ignored.md` (Xbox Live Avatar system; user declined the substitute-body port 2026-07-10) |
| 086 | AvatarMultipleAnimations | `AvatarMultipleAnimationsSample_4_0` | ❌ Ignored — see `ignored.md` (Xbox Live Avatar system; user declined the substitute-body port 2026-07-10) |
| 087 | AvatarShadows | `AvatarShadows_4_0` | ❌ Ignored — see `ignored.md` (Xbox Live Avatar system; user declined the substitute-body port 2026-07-10) |
| 088 | BingMaps | `BingMaps_4_0` | ❌ Ignored — see `ignored.md` (external web API) |
| 089 | BingMapsPathFinding | `BingMapsPathFinding_4_0` | ❌ Ignored — see `ignored.md` (external web API) |
| 090 | BitmapFontMaker | `BitmapFontMaker_4_0` | ❌ Ignored — see `ignored.md` (WinForms tool, not a `Game`) |
| 091 | ClientServerSample | `ClientServerSample_4_0` | ✅ Done (ported 2026-07-06; worked around 3 newly-found CNA gaps, see DEFERRED.md items 19–21) |
| 092 | ContentManifestExtensions | `ContentManifestExtensions_4_0` | ❌ Ignored — see `ignored.md` (content-pipeline extension, no executable) |
| 093 | CurveEditor | `CurveEditor_4_0` | ❌ Ignored — see `ignored.md` (WinForms tool) |
| 094 | CustomAvatarAnimation | `CustomAvatarAnimation_4_0` | ❌ Ignored — see `ignored.md` (Xbox Live Avatar system; user declined the substitute-body port 2026-07-10) |
| 095 | GeolocationSample | `GeolocationSample_4_0` | ❌ Ignored — see `ignored.md` (phone GPS hardware, no SDL equivalent) |
| 096 | InvitesSample | `InvitesSample_4_0` | ❌ Ignored — see `ignored.md` (Xbox Live account service) |
| 097 | MemoryMadnessLab | `MemoryMadnessLab_4_0` | ❌ Ignored — see `ignored.md` (WP7 teaching lab, not a standalone sample) |
| 098 | MicrophoneEcho | `MicrophoneEchoSample_4_0` | ✅ Done (ported 2026-07-06) |
| 099 | ModelImporterSample | `ModelImporterSample_4_0` | ❌ Ignored — see `ignored.md` (content-pipeline extension, no executable) |
| 100 | NetworkPrediction | `NetworkPredictionSample_4_0` | ✅ Done (ported 2026-07-10; confirmed zero networking workarounds needed, items 19–21 stay resolved; found `NetworkSession::SessionProperties` has no mutable/wire-replicated accessor, DEFERRED item #27 — see `samples/NetworkPrediction/missing.md`) |
| 101 | ObjectPlacementOnAvatar | `ObjectPlacementOnAvatarSample_4_0` | ❌ Ignored — see `ignored.md` (Xbox Live Avatar system; user declined the substitute-body port 2026-07-10) |
| 103 | PeerToPeer | `PeerToPeerSample_4_0` | ✅ Done (ported 2026-07-10; genuine peer-to-peer broadcast topology, no host authority — confirmed zero networking workarounds needed a third time; confirmed item #27 is sample-specific, not universal — see `samples/PeerToPeer/missing.md`) |
| 104 | PerformanceUtility | `PerformanceUtility_4_0` | ❌ Ignored — see `ignored.md` (utility library, no standalone executable) |
| 105 | PushNotifications | `PushNotificationsSample_4_0` | ❌ Ignored — see `ignored.md` (Windows Phone push notification service) |
| 106 | SavingEmbeddedImages | `SavingEmbeddedImages_4_0` | ❌ Ignored — see `ignored.md` (phone media library API) |
| 107 | TiltPerspective | `TiltPerspective_4_0` | ✅ Done (ported 2026-07-10, user-approved go/no-go; unlike #084, this original's own fallback is a non-interactive time-driven wobble with no keyboard branch to promote, so a keyboard-tilt scheme was genuinely invented from scratch — see `samples/TiltPerspective/missing.md`) |
| 108 | WinFormsContent | `WinFormsContentSample_4_0` | ❌ Ignored — see `ignored.md` (WinForms host window) |
| 109 | WinFormsGraphics | `WinFormsGraphicsSample_4_0` | ❌ Ignored — see `ignored.md` (WinForms host window) |
| 110 | WP7MusicManagement | `WP7MusicManagement_4_0` | ❌ Ignored — see `ignored.md` (WP7 media API, no desktop analog) |
| 111 | XnaGraphicsProfileChecker | `XnaGraphicsProfileChecker_4_0` | ❌ Ignored — see `ignored.md` (WinForms diagnostic tool) |

---

### Everything else (#112–153): permanently ignored — see `ignored.md`

The remaining 42 catalogued directories — XNA 2.0/3.0/3.1 archive samples (12),
Avatar asset/rig art packs with no C# code (7), phone/Mango platform variants that
duplicate a desktop sample already listed above (7), a VB language duplicate (1),
Silverlight/WP7-native directories with no XNA 4.0 code (5), image-only resource
directories (3), third-party/community kits that aren't official Microsoft samples
(3), unversioned/incomplete starter kits (2), and misc non-code directories (2) —
are none of them XNA 4.0 desktop samples this repo could ever build against CNA.
Full per-directory listing with reasons: [`ignored.md`](ignored.md).

---

## Phase Status Overview

**Note (2026-07-10): this table is known stale** (it was not kept in sync with
the per-phase porting done across several sessions — e.g. Phase 3's "Done"
count below has read 1 for a while despite LensFlare/Graphics3D/PickingSample/
TrianglePicking/HeightmapCollision/InverseKinematics/ChaseCamera all being
done). The **Sample Count Summary** table above (repo-wide totals) is kept
current each session; this per-phase breakdown is not. Treat it as historical
color, not an authoritative count — use the Sample Count Summary table and the
per-sample rows in the Complete Sample Task List above instead.

| Phase | Samples | Done | 🔓 Unblocked | 🚧 Placeholder | Ignored |
|---|---|---|---|---|---|
| Phase 1 — Foundation | 12 | 10 | 0 | 1 | 1 |
| Phase 2 — 2D Games | 18 | 16 | 0 | 2 | 0 |
| Phase 3 — 3D Graphics | 19 | 1 | 5 | 13 | 0 |
| Phase 4 — Models & Anim | 9 | 1 | 2 | 6 | 0 |
| Phase 5 — Audio | 2 | 2 | 0 | 0 | 0 |
| Phase 6 — Full Games | 14 | 8 | 1 | 3 | 2 |
| Phase 7 — Advanced / Misc | 10 | 8 | 0 | 1 | 1 |
| Deferred appendix (phone/Avatar/WinForms/Live) | 27 | 2 | 2 | 2 | 21 |
| Everything else (archives/art/dup/non-code — see `ignored.md`) | 42 | 0 | 0 | 0 | 42 |
| **Total** | **153** | **48** | **10** | **28** | **67** |

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
