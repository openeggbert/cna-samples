# NEXT.md — CNA Samples Handoff

## 1. Project Summary

**What:** C++ ports of the official Microsoft XNA Game Studio 4.0 sample collection,
running on **CNA** — a C++ reimplementation of the XNA 4.0 API built on SDL3.

**Main goal:** Port all ~83 applicable XNA 4.0 desktop samples to CNA C++, preserving
the original class hierarchy and naming (`Microsoft::Xna::Framework::*`), serving as
living integration tests for the CNA framework and as a migration reference.

**Current phase:** Phases 1 (Foundation), 2 (2D Games) and 5 (Audio) are complete except
items deferred on missing CNA features. Phase 6 (Full games) is in progress
(GameStateManagement #072, CatapultWars #067, and Yacht #071 done). Phases 3 (3D shaders)
and 4 (models/animation) are untouched and blocked on missing asset pipelines.

**Key architectural decisions:**
- One executable per sample; no shared sample library. Each sample is self-contained.
- All sample code is header-only except `Program.cpp`.
- Assets: `Load<Texture2D>` → PNG; `Load<SpriteFont>` → `.font.json` + PNG atlas (via
  `tools/make_font.py`); `Load<SoundEffect>` → WAV; `Load<Song>` → OGG/WAV/MP3/FLAC.
  XNA `.xnb` is never supported.
- `CONTENT_DIR` in `cna_add_sample()` is **mandatory** for any sample that loads assets
  (including the F1 `help.png` overlay) — otherwise the binary aborts at startup.
- Default graphics backend is **EasyGL** (OpenGL ES); a Vulkan backend also exists.
- CNA is consumed via `add_subdirectory(../cna)`, so building a sample rebuilds CNA.

---

## 2. Current Status

### Build
All enabled samples compile and link cleanly with the default **EasyGL** backend:
`cmake --build cmake-build-debug` → green. The active build tree is `cmake-build-debug`.

### Tests
No automated test suite in this repo — the samples themselves are the manual/visual
integration tests. Verification is by running a sample and inspecting a screenshot.

### Enabled samples (31)
| #   | Sample               | Status  | Notes |
|-----|----------------------|---------|-------|
| 001 | PrimitivesSample     | ✅ runs | 2D DrawUserPrimitives |
| 002 | Primitives3D         | ✅ runs | DiffuseColor (B) + wireframe (Y) now work on EasyGL |
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
| 016 | Bounce               | ✅ runs | lit 3D spheres |
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
| 071 | Yacht                | ✅ runs | full dice game: real TouchPanel gestures + real Accelerometer (SDL3-backed) + mouse; portrait 480×800; online multiplayer (WCF) dropped |
| 072 | GameStateManagement  | ✅ runs | menu/screen framework; multiple SpriteBatch/frame OK on EasyGL |

### Deferred (not built — no CMakeLists yet)
| #   | Sample            | Blocker |
|-----|-------------------|---------|
| 005 | ReachGraphicsDemo | Model + SkinnedAnimation + SpriteFont menus |
| 014 | Spacewar          | Model + custom shaders + XACT |
| 028 | ColorReplacement  | Model + custom Effect |
| 031 | BloomSample       | custom Effect (3 shaders) + RenderTarget2D + Model |

### What does not work yet
- **CameraShake 3D scene**: white stripe on all backends (near-plane clipping of w<0).
- **Phases 3 & 4** (3D shaders / models + animation): no sample ported; need an
  HLSL→GLSL shader workflow and a model-conversion pipeline (neither exists in this repo).
- **Vulkan backend**: multiple `SpriteBatch.Begin/End` per frame discards the first (EasyGL,
  the default, is fine).

---

## 3. Recent Changes

- **samples/Yacht/** (new, #071) — Full port of the XNA "Yacht Starter Kit" Windows
  Phone dice game. 17 header-only files + `Program.cpp`. Unlike CatapultWars/
  GameStateManagement (which remapped touch to mouse only), this port uses CNA's
  **real** `TouchPanel`/`GestureDetector` (SDL3 finger events, all `GestureType`
  values actually detected) and CNA's **real** `Microsoft::Devices::Sensors::Accelerometer`
  (SDL3 `SDL_Sensor`) for shake-to-roll, with mouse added as an explicit parallel
  path everywhere the original was touch-only (Roll/Score buttons, dice pick-up,
  scorecard tap/scroll). A pre-existing `NEXT.md`/`CLAUDE.md` claim that CNA lacked
  touch input and SpriteFont was **stale** — both are fully implemented; both docs
  corrected. Dice roll animation ports the original's shared-RNG/timed-reroll
  behaviour from a background `System.Threading.Timer` to a per-die elapsed-time
  accumulator (no threads). Online multiplayer (WCF service + Windows Phone push
  notifications) and the tombstoning save/load lifecycle are dropped entirely —
  local Human-vs-3-AI play is fully self-contained and was confirmed faithful to
  the original (same player names "Josh"/"Charles"/"Alex", same 12-category
  scorecard, same turn/scoring rules). One deliberate behavioural improvement: the
  original's keyboard-arrow accelerometer fallback was dead code off-device (nothing
  ever polled `Accelerometer.GetState()` outside the real-hardware event handler) —
  the port polls it every frame so shake-to-roll actually works on a desktop with
  no accelerometer. Screenshot- and human-input-verified: main menu (no "Online
  Game" entry), offline game start (correct 4 players), a real dice roll, holding
  dice (tap/click), the scorecard's live per-category score preview, and the
  keyboard-arrow shake-to-roll fallback. See `samples/Yacht/missing.md` for the
  full list of differences from the original, including two assets (`button.png`,
  `Dot.png`) an initial pass wrongly flagged as unused due to a grep bug (missed
  a forward-slash path style and a case difference in the original's own source)
  — caught and fixed before commit.
- **Interactive input verification (GameStateManagement + CatapultWars)** — launched both
  binaries under `SDL_VIDEODRIVER=x11` (XWayland) on a real desktop session and drove them
  with genuine human keyboard/mouse input (not the auto-advance hack used for earlier
  screenshots), confirming via screenshots between steps: **GameStateManagement** — Down
  arrow moves the Main Menu highlight, Enter selects "Play Game" into the GameplayScreen,
  Escape opens the PauseMenuScreen ("Resume Game"/"Quit Game"), and a mouse click on a menu
  entry ("Options") navigates correctly; **CatapultWars** — the title screen's "Play" button
  leads through InstructionsScreen into a level, and a real mouse press-drag-release fires
  the catapult per the "Drag Anywhere to Fire" prompt. No regressions found. (Tooling note:
  `xdotool` had to be installed for window activation; real synchronized-frame screenshots
  of a human-driven session are inherently racy over chat round-trips, so a couple of
  transitions were confirmed by direct user report rather than a mid-transition screenshot.)
- **samples/CatapultWars/** (new, #067) — Full port of the XNA "Catapult Wars Lab" Windows
  Phone game (EX2/End). 14 header-only files + `Program.cpp`: reuses the
  GameStateManagement ScreenManager framework; adds `Animation` (sprite-sheet),
  `AudioManager` (SoundEffect bank), `Projectile`/`Catapult` (bit-flag state machine,
  BoundingSphere/Box collision), `Player`/`Human`/`AI`, and 5 screens. Touch gestures
  mapped to mouse drag/click; fixed 30 fps timestep; animation defs inlined from
  `AnimationsDef.xml`. Fonts from DejaVuSans-Bold. Authored `CatapultWars.htm` (the training
  kit ships a `.doc`, not a `.htm`). **No CNA or sharp-runtime changes were needed** —
  this sample is a pure consumer of existing API. Screenshot-verified (menu + gameplay).
- **cna repo (EasyGL backend)** — Fixed two Primitives3D bugs (committed + pushed to
  `openeggbert/cna`): (1) **DiffuseColor** — `EnsureColored3DProgram()` now outputs
  `vColor * uDiffuseColor` and wires `loc_diffuse`; the non-Ex user-primitive paths upload
  white so the uniform never defaults to 0/black. (2) **Wireframe** — `ApplyRasterizerState`
  sets a `wireframe_` flag and the 3D draw paths re-expand triangles into `GL_LINES` via a
  new `DrawWireframe` helper. DEFERRED.md items 3 & 4 closed.
- **samples (CONTENT_DIR audit)** — Several enabled samples shipped `Content/help.png` but
  lacked `CONTENT_DIR`, so the F1 overlay asset was never copied next to the binary and the
  sample aborted at startup. Added `CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Content` to
  **Primitives3D, PrimitivesSample, ShapeRendering, Bounce, CollisionSample** (all
  re-verified running). The 4 deferred samples with the same gap have no `CMakeLists.txt`
  yet — fix when they are enabled.
- **Docs** — NEXT.md, PLAN.md (Phase 6: 2 done, total 30), DEFERRED.md, and the affected
  `missing.md` files updated.

---

## 4. Current Blocker / Main Problem

There is **no hard blocker** stopping all progress — portable 2D samples remain in Phase
6/7. The most important *standing bug* and the main *capability gap* are:

### A. CameraShake 3D white stripe (confirmed bug, all backends)
- **Symptom:** The 3D scene renders as a thin bright bar; tank and ground invisible.
- **Reproduce:** `./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples`
- **Affected:** `samples/CameraShake/src/CameraShakeGame.hpp` (camera at (1000,1000,1000),
  ground scale 0.1 → vertices at ±6554); CNA near-plane clipping of w<0 vertices in the
  EasyGL/Vulkan backends.
- **Suspected cause:** CNA does not clip primitives against the near plane the way DirectX
  does, so vertices with w<0 are projected incorrectly.
- **Tried:** Removed an incorrect z-remap from the Vulkan 3D vertex shaders (correct, but no
  visual change); a scale-0.02 workaround was rejected (do not hide the bug in the sample);
  EasyGL shows the same stripe. Fix belongs in CNA's shared clipping logic.

### B. Phase 3/4 pipeline gap (capability gap, blocks ~28 samples)
- Phase 3 needs custom HLSL effects translated to GLSL `.shader.json`; Phase 4 needs
  model conversion to `.model.json` plus (for several) skeletal animation. No proven
  workflow for either exists in this repo yet.

---

## 5. Known Bugs and Limitations

| Label | Description |
|-------|-------------|
| confirmed bug | **CameraShake white stripe (all backends).** CNA near-plane clipping of w<0 vertices differs from DirectX. |
| confirmed bug | **Multiple SpriteBatch/frame on Vulkan.** Second `Begin/End` discards the first. Works on EasyGL (default). Affects multi-layer samples (e.g. GameStateManagement, CatapultWars) if run on Vulkan. |
| incomplete | **Phase 3 (3D shaders)** — 19 samples blocked on HLSL→GLSL translation. |
| incomplete | **Phase 4 (Models/Anim)** — 9 samples blocked on a model-conversion + animation pipeline. |
| incomplete | **4 deferred samples have no CMakeLists.txt** (BloomSample, ColorReplacement, ReachGraphicsDemo, Spacewar) and ship `help.png` without `CONTENT_DIR` — add both when enabling them. |
| limitation | **No SpriteFont `.xnb` pipeline.** Atlases must be generated with `tools/make_font.py` (and `"Moire ExtraBold"` etc. are substituted with DejaVu fonts). |
| corrected | ~~No touch input~~ — **stale.** CNA has a real `Microsoft::Xna::Framework::Input::Touch::TouchPanel` (all `GestureType` values actually detected by `CNA::Internal::Input::GestureDetector`, SDL3 finger events wired in `SdlInputBridge.cpp`) and a real `Microsoft::Devices::Sensors::Accelerometer` (SDL3 `SDL_Sensor`-backed). Earlier ports (CatapultWars, GameStateManagement) predated this and chose to remap touch to mouse instead of using it; that was a per-sample choice, not a CNA limitation. Now proven end-to-end by Yacht #071 (real gestures + real accelerometer, keyboard-fallback path verified). Gotcha: a sample must call `TouchPanel::setDisplayWidthProperty/setDisplayHeightProperty` itself (no auto-wiring from the backbuffer size) or touch coordinates come out wrong. |
| corrected | ~~SpriteFont deferred, omit DrawString~~ — **stale**, was in `CLAUDE.md` (fixed). SpriteFont has been ✅ resolved for a while (DEFERRED.md item 2) — `.font.json` + PNG atlas via `tools/make_font.py`, `SpriteBatch.DrawString` fully implemented. Don't omit text in new ports. |
| verified | **GameStateManagement + CatapultWars interactive input** — confirmed on a real desktop session (SDL_VIDEODRIVER=x11/XWayland + human-driven keyboard/mouse, screenshots between steps). See Recent Changes. |
| verified | **Yacht real touch/gesture + real accelerometer input** — confirmed via screenshots + human-driven interaction: main menu, offline game start, dice roll, dice hold, scorecard preview, keyboard-arrow shake-to-roll fallback. Real hardware accelerometer not tested (no sensor on this desktop). |

---

## 6. Architecture Notes

### Sample directory layout
```
samples/SampleName/
  src/Program.cpp        # int main(){ NS::GameClass g; g.Run(); return 0; }
  src/*.hpp              # all logic inline (header-only)
  Content/               # copied next to the binary via CONTENT_DIR (POST_BUILD)
  CMakeLists.txt         # cna_add_sample(name SOURCES src/Program.cpp CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Content)
  missing.md             # required: differences vs the XNA original
  SampleName.htm         # required: MS documentation (authored if the kit lacks one)
```

### CNA API rules — must not break
- All XNA properties use `getXxxProperty()` / `setXxxProperty()` — never raw member access.
- Every `Game` **and** `DrawableGameComponent` subclass implements
  `const std::string& GetTypeName() const override`.
- `Content.Load<T>()` returns **T by value** — store in `std::optional<T>`. Textures keeping
  a pointer (e.g. an `Animation` referencing a sheet) must store the texture somewhere with
  a **stable address** (`std::map` node / `std::optional`, not a reallocating `std::vector`).
- Virtual overrides: `void Update(GameTime&)` (non-const) and `void Draw(const GameTime&)`
  (const). `DrawableGameComponent::Initialize()` calls `LoadContent()` once.
- No `operator+=` for `Vector2`/`TimeSpan` — write `v = v + expr`. `Color` has no `operator*`
  — multiply channels manually (see the `mul()` helper in GameScreen.hpp).
- `Rectangle` exposes public `X/Y/Width/Height` and `Contains(x, y)` (no `getXProperty`).
- `Point` exposes public `X/Y`. `Vector2` exposes public `X/Y`.
- `CONTENT_DIR` must be `${CMAKE_CURRENT_SOURCE_DIR}/Content` (the POST_BUILD copy runs from
  the binary dir, so a bare relative path fails).
- SpriteFont atlases must be generated with `--content-root samples/X/Content` so the
  `.font.json` `texture` path is relative to the Content root.

### ScreenManager framework (shared shape of GameStateManagement & CatapultWars)
- `Game` owns a `ScreenManager` (DrawableGameComponent, added to `Components`).
- `ScreenManager` holds a stack of `std::shared_ptr<GameScreen>`; `Update` copies the stack,
  routes input to the topmost active screen, and lets each screen transition.
- **Invariant:** a screen may remove itself during its own `Update` (via `ExitScreen` →
  `RemoveScreen(this)`); the `shared_ptr` copy in the update loop keeps it alive until the
  iteration ends. Do **not** switch screen ownership to raw owning pointers — that
  reintroduces use-after-free.
- Cross-referencing screens (screen A creates screen B which creates A) are resolved with
  forward declarations + out-of-line inline definitions at the bottom of `Screens.hpp`.

### CatapultWars specifics
- `Catapult` ↔ `Player` is a cycle: `Catapult` holds non-owning `Player*` back-pointers and
  declares the Player-dependent methods (`CheckHit`, `getEnemyScore`), which are **defined
  out-of-line in `Player.hpp`** after `Player` is complete. `Player` owns its `Catapult` via
  `std::shared_ptr`.
- `CatapultState` is a bit-flag `enum class` with `constexpr operator|`/`operator&`; the
  combined `Firing | ProjectileFlying` state is a real switch case.
- Animations advance one frame per `Update`; the game runs at a **fixed 30 fps timestep**
  (`setTargetElapsedTimeProperty(FromSeconds(1.0/30.0))`) or they play twice as fast.

---

## 7. Useful Commands

```bash
# Configure (first time)
cmake --preset debug            # or: cmake --preset debug-easygl

# Build everything / one sample (rebuilds CNA as needed)
cmake --build cmake-build-debug
cmake --build cmake-build-debug --target CatapultWars_cna_samples

# Run the newest sample (run from its own dir so Content/ is found)
cd cmake-build-debug/samples/CatapultWars && ./CatapultWars_cna_samples

# Reproduce the main bug (CameraShake white stripe)
./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples

# Generate a SpriteFont atlas (ALWAYS pass --content-root)
python3 tools/make_font.py /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf 24 \
    samples/X/Content/Fonts/MenuFont --content-root samples/X/Content

# Generate the F1 help overlay PNG from the sample .htm
python3 tools/gen_help_png.py samples/X/X.htm samples/X/Content/help.png

# Headless screenshot of an SDL3 app (native-Wayland windows are invisible to X11 tools).
# NOTE: there is no input-injection tool installed (no xdotool/xte); to screenshot a
# non-initial screen, temporarily auto-advance in code, capture, then revert.
SDL_VIDEODRIVER=x11 ./cmake-build-debug/samples/X/X_cna_samples &
WID=$(xwininfo -root -tree | grep -i X_cna_samples | grep -oE '0x[0-9a-f]+' | head -1)
import -window "$WID" /tmp/shot.png
```

---

## 8. Next Smallest Tasks

1. **Port the next portable 2D sample.**
   - Goal: keep extending coverage. Now that real `TouchPanel`/`Accelerometer` are
     confirmed working end-to-end (see Yacht #071), touch/accelerometer samples are
     no longer a reason to deprioritize a candidate — pick based on remaining asset
     needs (SpriteFont/audio proven; 3D shaders/models still blocked, see below).
   - Files: new `samples/<Name>/`; root `CMakeLists.txt`.
   - Verify: `cmake --build cmake-build-debug --target <Name>_cna_samples` + screenshot.

2. **Investigate CameraShake near-plane clipping in the EasyGL backend.**
   - Goal: clip w<0 vertices the way DirectX does so the ground/tank render.
   - Files: `cna/.../EasyGL/EasyGLGraphicsBackend.cpp` (clipping/projection path).
   - Verify: the white stripe disappears in `CameraShake_cna_samples`.

3. **Fix the Vulkan multiple-SpriteBatch-per-frame bug.**
   - Goal: a second `Begin/End` in the same frame must not discard the first.
   - Files: `cna/.../Vulkan/VulkanGraphicsBackend.cpp`.
   - Verify: run GameStateManagement/CatapultWars on the Vulkan backend; all layers draw.

4. **Add CMakeLists + CONTENT_DIR to a deferred sample when its blocker is lifted.**
   - Goal: when a Model/Effect pipeline exists, enable one of BloomSample / ColorReplacement
     / ReachGraphicsDemo / Spacewar with `CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Content`.
   - Files: `samples/<Name>/CMakeLists.txt`, root `CMakeLists.txt`.
   - Verify: build target + run from the binary dir (must not abort on `help`).

5. **Verify real hardware accelerometer shake on a device that has one.**
   - Goal: Yacht's shake-to-roll was only verified via the keyboard-arrow fallback
     on this desktop (no physical accelerometer). The real-sensor code path
     (`Accelerometer::getIsSupportedProperty()` true branch) is implemented but
     untested end-to-end.
   - Files: `samples/Yacht/src/Accelerometer.hpp`.
   - Verify: run on a machine/device with a real accelerometer; shake it.

---

## 9. Do Not Do Yet

- **Do not start Phase 3 samples** without first establishing an HLSL→GLSL `.shader.json`
  workflow — no HLSL runtime compiler exists in CNA.
- **Do not start Phase 4 samples** without a proven model-conversion path to `.model.json`.
- **Do not add a shared `samples/common/` library** — each sample stays self-contained.
- **Do not change the CameraShake ground scale / camera** to hide the white stripe — fix the
  CNA clipping bug instead.
- **Do not work around CNA bugs inside samples** (no per-control scaling, sample-side
  letterboxing, etc.). If CNA diverges from XNA 4.0, check FNA
  (`/rv/data/library/github.com/FNA-XNA/FNA`) and fix it in CNA.
- **Do not switch ScreenManager screen ownership** from `shared_ptr` to raw pointers.
- **Do not re-generate existing font atlases** unless there is a confirmed rendering bug.
- **No broad refactors or unrelated cleanup** while the CameraShake clipping bug is open.

---

## 10. Resume Prompt

```
Read NEXT.md first to understand current project state.
Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not start any task not listed in section 8.
Make one small, verified improvement, then run the relevant build/screenshot command
(e.g. cmake --build cmake-build-debug --target <Name>_cna_samples) to confirm it.
After finishing, update NEXT.md (status, recent changes, next tasks) to match reality.
```
