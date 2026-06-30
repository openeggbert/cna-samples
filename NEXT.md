# NEXT.md — CNA Samples Handoff

## 1. Project Summary

**What:** C++ ports of the official Microsoft XNA Game Studio 4.0 sample collection,
running on CNA — a C++ reimplementation of the XNA 4.0 API built on SDL3.

**Main goal:** Port all ~83 applicable XNA 4.0 desktop samples to CNA C++, preserving
the original class hierarchy and naming (`Microsoft::Xna::Framework::*`), serving as
living integration tests for the CNA framework and as a migration reference.

**Current phase:** Phase 1 (Foundation) and Phase 2 (2D Games) complete except items
deferred on missing CNA features. Phase 5 (Audio) complete (2/2). Phase 6 just started
(GameStateManagement / #072 done). Phases 3, 4, and most of 7 untouched.

**Key architectural decisions:**
- One executable per sample; no shared sample library. Each sample is self-contained.
- All sample code is header-only (except `Program.cpp`).
- Assets: `Load<Texture2D>()` → PNG; `Load<SpriteFont>()` → `.font.json` + PNG atlas
  (via `tools/make_font.py`); `Load<SoundEffect>()` → WAV; `Load<Song>()` → OGG/WAV/MP3/FLAC.
  XNA `.xnb` is never supported.
- `CONTENT_DIR` argument to `cna_add_sample()` is mandatory for any sample loading assets.
- Default graphics backend is now **EasyGL** (OpenGL ES); a Vulkan backend also exists.

---

## 2. Current Status

### Build
All enabled samples compile and link cleanly (`cmake --build cmake-build-debug` → 57
targets green). The active build tree `cmake-build-debug` currently runs the **EasyGL**
backend.

### Tests
No automated test suite in this repo — the samples themselves are the manual/visual
integration tests. Verification is by running a sample and inspecting it (screenshots).

### Enabled samples (30)
| #   | Sample               | Status  | Notes |
|-----|----------------------|---------|-------|
| 001 | PrimitivesSample     | ✅ runs | 2D DrawUserPrimitives |
| 002 | Primitives3D         | ✅ runs | DiffuseColor (B) and wireframe (Y) now work on EasyGL |
| 003 | TexturesAndColors    | ✅ runs | |
| 006 | SpriteEffects        | ✅ runs | |
| 007 | SpriteSheet          | ✅ runs | |
| 008 | ShapeRendering       | ✅ runs | |
| 009 | InputReporter        | ✅ runs | gamepad display, 6 SpriteFont sizes |
| 010 | InputSequence        | ✅ runs | |
| 011 | SafeArea             | ✅ runs | |
| 012 | GeneratedGeometry    | ✅ runs | |
| 013 | Platformer           | ✅ runs | full game: 3 levels, player, enemies, gems, HUD |
| 015 | TicTacToe            | ✅ runs | |
| 016 | Bounce               | ✅ runs | |
| 017 | CollisionSample      | ✅ runs | |
| 018 | PerPixelCollision    | ✅ runs | |
| 019 | RectangleCollision   | ✅ runs | |
| 020 | TransformedCollision | ✅ runs | |
| 021 | PathDrawing          | ✅ runs | |
| 022 | Pathfinding          | ✅ runs | |
| 023 | WaypointSample       | ✅ runs | |
| 024 | FlockingSample       | ✅ runs | |
| 025 | ChaseAndEvade        | ✅ runs | |
| 026 | AimingSample         | ✅ runs | |
| 027 | FuzzyLogic           | ✅ runs | |
| 029 | ParticleSample       | ✅ runs | |
| 030 | CameraShake          | ⚠️ runs | 3D scene renders as a white stripe (see blocker) |
| 059 | Audio3D              | ✅ runs | 3D positional audio; alpha-test billboards |
| 060 | SoundAndMusic        | ✅ runs | portrait 480×800; SoundEffect + Song; mouse sliders |
| 067 | CatapultWars         | ✅ runs | full 2D game: menus, AI, sprite-sheet animation, SoundEffect, HUD |
| 072 | GameStateManagement  | ✅ runs | menu/screen framework; multiple SpriteBatch/frame OK on EasyGL |

### Deferred (not built)
| #   | Sample           | Blocker |
|-----|------------------|---------|
| 005 | ReachGraphicsDemo | Model + SkinnedAnimation + SpriteFont menus |
| 014 | Spacewar         | Model + custom shaders + XACT |
| 028 | ColorReplacement | Model + custom Effect |
| 031 | BloomSample      | custom Effect (3 shaders) + RenderTarget2D + Model |

### What does not work yet
- **CameraShake 3D scene**: white stripe on all backends (near-plane clipping of w<0).
- **Phases 3 & 4** (3D shaders / models + animation): no sample ported; need an
  HLSL→GLSL workflow and a model-conversion pipeline (neither exists yet).

---

## 3. Recent Changes

- **samples/CatapultWars/** (new, #067) — Full port of the XNA "Catapult Wars Lab"
  Windows Phone game (EX2/End). 14 header-only files + Program.cpp: the ScreenManager
  framework (reused from GameStateManagement), `Animation` (sprite-sheet), `AudioManager`
  (SoundEffect bank), `Projectile`/`Catapult` (bit-flag state machine, BoundingSphere/Box
  collision), `Player`/`Human`/`AI`, and 5 screens (Background, MainMenu, Instructions,
  Pause, Gameplay). Touch gestures adapted to mouse drag/click; fixed 30fps timestep;
  animation defs inlined from AnimationsDef.xml. Fonts MenuFont/HUDFont generated from
  DejaVuSans-Bold. Screenshot-verified: main menu + full gameplay (HUD, wind, both
  catapults, parallax clouds). Authored CatapultWars.htm (training kit has no .htm).
- **cna repo (EasyGL backend)** — Two Primitives3D bugs fixed + verified (screenshots):
  (1) **DiffuseColor** — `EnsureColored3DProgram()` now outputs `vColor * uDiffuseColor`
  and wires `loc_diffuse`; the non-Ex user-primitive paths upload white explicitly so the
  uniform never defaults to 0/black. (2) **Wireframe** — `ApplyRasterizerState` sets a
  `wireframe_` flag and the 3D draw paths re-expand triangles to `GL_LINES` via the new
  `DrawWireframe` helper (scratch 32-bit line index buffer; indexed + non-indexed list/strip).
  DEFERRED.md items 3 & 4 closed.
- **samples (CONTENT_DIR audit)** — Several enabled samples had `Content/help.png` but no
  `CONTENT_DIR` in their `CMakeLists.txt`, so the F1 overlay asset was never copied next to
  the binary and the sample **aborted at startup** (only "ran" when launched from the source
  dir). Added `CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Content` to **Primitives3D,
  PrimitivesSample, ShapeRendering, Bounce, CollisionSample** (all re-verified running).
  The 4 deferred samples with the same gap (BloomSample, ColorReplacement, ReachGraphicsDemo,
  Spacewar) have no `CMakeLists.txt` yet — fix when they are enabled.
- **samples/GameStateManagement/** (new, #072) — Full port of the canonical XNA GSMSample
  (Win+Xbox): `ScreenManager` (DrawableGameComponent), `GameScreen` transition state
  machine, `InputState`, `MenuScreen`/`MenuEntry`, and all screens (Background, MainMenu,
  Options, Pause, MessageBox, Loading, Gameplay). Screens owned via `std::shared_ptr` to
  mirror C# GC semantics. Screenshot-verified (Main Menu, Options with working toggles,
  Gameplay). Confirms multiple SpriteBatch Begin/End per frame work on EasyGL.
- **cna repo (GraphicsDeviceManager.cpp)** — Fixed portrait-orientation bug FNA-faithfully:
  `supportsOrientations_` is now platform-gated via `SDL_GetPlatform()` (true only on
  iOS/Android, matching FNA). Desktop now honors `PreferredBackBufferWidth × Height`
  verbatim (480×800 → portrait). Committed + pushed to `openeggbert/cna` develop.
- **samples/SoundAndMusic/** (#060) — SoundEffect fire-and-forget, looped
  SoundEffectInstance with pan/pitch/volume sliders, Song play/pause/stop. Portrait
  480×800; touch adapted to mouse.
- **PLAN.md** — status table corrected (Phase 2: 16, Phase 5: 2, Phase 6: 1; total 29 done).

---

## 4. Current Blocker / Main Problem

There is **no hard blocker** stopping all progress — portable 2D samples remain in
Phase 6/7. The most important *standing bug* and the main *capability gap* are:

### A. CameraShake 3D white stripe (confirmed bug, all backends)
- **Symptom:** The 3D scene renders as a thin bright bar; tank and ground invisible.
- **Reproduce:** `./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples`
- **Affected:** `samples/CameraShake/src/CameraShakeGame.hpp` (camera at (1000,1000,1000),
  ground scale 0.1 → vertices at ±6554); CNA near-plane clipping of w<0 vertices.
- **Tried:** Removed an incorrect z-remap from the Vulkan 3D vertex shaders (correct but no
  visual change); tested a scale-0.02 workaround (reverted to keep XNA values); EasyGL shows
  the same stripe. Fix belongs in CNA's shared clipping logic, not the sample.

### B. Phase 3/4 pipeline gap (capability gap, blocks ~28 samples)
- Phase 3 samples need custom HLSL effects translated to GLSL `.shader.json`; Phase 4 needs
  `.fbx/.x` model conversion to `.model.json` plus (for several) skeletal animation. No
  proven workflow for either exists in this repo yet.

---

## 5. Known Bugs and Limitations

| Label | Description |
|-------|-------------|
| confirmed bug | **CameraShake white stripe (all backends).** CNA near-plane clipping of w<0 vertices differs from DirectX. |
| fixed | **Primitives3D DiffuseColor (EasyGL).** `prog_colored_` now multiplies `vColor * uDiffuseColor`. |
| fixed | **Primitives3D wireframe (EasyGL).** `FillMode::WireFrame` emulated by re-expanding triangles to `GL_LINES` (`DrawWireframe`). |
| confirmed bug | **Multiple SpriteBatch/frame on Vulkan.** Second `Begin/End` discards the first. Works on EasyGL (default). Affects any multi-layer sample (e.g. GameStateManagement) if run on Vulkan. |
| incomplete | **Phase 3 (3D shaders)** — 19 samples blocked on HLSL→GLSL translation. |
| incomplete | **Phase 4 (Models/Anim)** — 9 samples blocked on model conversion + animation. |
| needs verification | **GameStateManagement interactive input.** Headless x11 testing injected synthetic key events that auto-navigated menus; verify real keyboard/gamepad navigation on a desktop session. |

---

## 6. Architecture Notes

### Sample directory layout
```
samples/SampleName/
  src/Program.cpp        # int main(){ NS::GameClass g; g.Run(); return 0; }
  src/*.hpp              # all logic inline (header-only)
  Content/               # copied next to the binary via CONTENT_DIR
  CMakeLists.txt         # cna_add_sample(name SOURCES src/Program.cpp CONTENT_DIR ...)
  missing.md             # required: differences vs the XNA original
  SampleName.htm         # required: verbatim MS documentation
```

### CNA API rules — must not break
- All XNA properties: `getXxxProperty()` / `setXxxProperty()` — never raw member access.
- Every `Game` subclass implements `const std::string& GetTypeName() const override`.
- `Content.Load<T>()` returns **T by value** — store in `std::optional<T>`.
- Virtual overrides: `void Update(GameTime&)` (non-const) and `void Draw(const GameTime&)`
  (const). `DrawableGameComponent::Initialize()` calls `LoadContent()` once.
- No `operator+=` for `Vector2`/`TimeSpan` — write `v = v + expr`. `Color` has no
  `operator*` — multiply channels manually (see GameStateManagement `mul()` helper).
- `GraphicsDeviceManager.SupportedOrientations`/portrait now honored on desktop (FNA-style).
- SpriteFont atlases must be generated with `--content-root samples/X/Content` so the
  `.font.json` `texture` path is relative to the Content root.

### GameStateManagement (most complex sample) data flow
- `Game` → owns a `ScreenManager` (DrawableGameComponent, added to `Components`).
- `ScreenManager` holds a stack of `std::shared_ptr<GameScreen>`; `Update` copies the stack,
  routes input to the topmost active screen, and lets each screen transition.
- **Invariant:** a screen may remove itself during its own `Update` (via `ExitScreen` →
  `RemoveScreen(this)`); the `shared_ptr` copy in the update loop keeps it alive until the
  iteration ends. Do not switch screens to raw owning pointers — that reintroduces UAF.

---

## 7. Useful Commands

```bash
# Configure (first time)
cmake --preset debug            # or: cmake --preset debug-easygl

# Build everything / one sample
cmake --build cmake-build-debug
cmake --build cmake-build-debug --target GameStateManagement_cna_samples

# Run the newest sample
./cmake-build-debug/samples/GameStateManagement/GameStateManagement_cna_samples

# Reproduce the main bug (CameraShake white stripe)
./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples

# Generate a SpriteFont atlas (ALWAYS pass --content-root)
python3 tools/make_font.py /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf 23 \
    samples/X/Content/menufont --content-root samples/X/Content

# Generate the F1 help overlay
python3 tools/gen_help_png.py samples/X/X.htm samples/X/Content/help.png

# Headless screenshot of an SDL3 app (native-Wayland windows are invisible to X11 tools):
#   run with the X11 driver, find the window id, capture with ImageMagick
SDL_VIDEODRIVER=x11 ./cmake-build-debug/samples/X/X_cna_samples &
WID=$(xwininfo -root -tree | grep -i X_cna_samples | grep -oE '0x[0-9a-f]+' | head -1)
import -window "$WID" /tmp/shot.png
```

---

## 8. Next Smallest Tasks

1. **Verify GameStateManagement interactive input on a real desktop session.**
   - Goal: confirm Up/Down/Enter/Esc (and gamepad) navigate menus correctly; the headless
     harness injected synthetic events, so this is unverified with real input.
   - Files: `samples/GameStateManagement/src/InputState.hpp`.
   - Verify: run the binary, navigate Main → Options → Back, Play Game → Esc → Pause → Quit.

2. ~~**Fix Primitives3D DiffuseColor on EasyGL.**~~ ✅ DONE — `EnsureColored3DProgram()`
   now outputs `vColor * uDiffuseColor` and wires `loc_diffuse`.  Screenshot-verified:
   the cube renders red (DiffuseColor) instead of white.

3. ~~**Fix Primitives3D wireframe on EasyGL.**~~ ✅ DONE — `ApplyRasterizerState` sets a
   `wireframe_` flag; the 3D draw paths re-expand triangles to `GL_LINES` via the new
   `DrawWireframe` helper.  Screenshot-verified: the cube renders as an edge-only wireframe.
   (Also fixed: Primitives3D `CMakeLists.txt` was missing `CONTENT_DIR`, so `Content/help.png`
   was never copied and the sample aborted at startup.)

4. **Investigate CameraShake near-plane clipping in the EasyGL backend.**
   - Goal: clip w<0 vertices the way DirectX does so the ground/tank render.
   - Files: `cna/.../EasyGL/EasyGLGraphicsBackend.cpp` (clipping/projection path).
   - Verify: white stripe disappears in `CameraShake_cna_samples`.

5. ~~**Port the next 2D Phase 6/7 sample.**~~ ✅ DONE — CatapultWars #067 ported and
   screenshot-verified (menu + gameplay). **Yacht #071** is the next 2D candidate, but it
   is MODERATE: touch-only gameplay (more remapping) plus an accelerometer and a WCF
   online-multiplayer server to drop (the local vs-AI mode ports cleanly). Other proven-
   feature 2D/SpriteFont samples are also fair game.

---

## 9. Do Not Do Yet

- **Do not start Phase 3 samples** without first establishing an HLSL→GLSL `.shader.json`
  workflow — no HLSL runtime compiler exists in CNA.
- **Do not start Phase 4 samples** without a proven `.fbx/.x` → `.model.json` conversion path.
- **Do not add a shared `samples/common/` library** — each sample stays self-contained.
- **Do not change the CameraShake ground scale / camera** to hide the white stripe — fix the
  CNA clipping bug instead.
- **Do not work around CNA bugs inside samples** (no per-control scaling, sample-side
  letterboxing, etc.). If CNA diverges from XNA 4.0, check FNA
  (`/rv/data/library/github.com/FNA-XNA/FNA`) and fix it in CNA.
- **Do not switch GameStateManagement screen ownership** from `shared_ptr` to raw pointers.
- **Do not re-generate existing font atlases** unless there is a confirmed rendering bug.

---

## 10. Resume Prompt

```
Read NEXT.md first to understand current project state.
Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not start any task not listed in section 8.
Make one small, verifiable improvement.
Run the relevant build/run command from section 7 and confirm it works
(screenshot-verify UI changes; native-Wayland windows need the SDL_VIDEODRIVER=x11 trick).
Update NEXT.md after finishing to reflect what changed.
```
