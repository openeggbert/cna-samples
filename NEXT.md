# NEXT.md

## 1. Project summary

**What this is:** C++ ports of the official Microsoft XNA Game Studio 4.0 sample
collection, running on **CNA** (`../cna`) — a C++ reimplementation of the XNA 4.0 API
built on SDL3, itself built on **sharp-runtime** (`../sharp-runtime`), a C++ port of
relevant .NET BCL types.

**Main goal:** Port all ~83 applicable XNA 4.0 desktop samples to CNA C++, preserving the
original class hierarchy and naming (`Microsoft::Xna::Framework::*`). The ported samples
double as living integration tests for the CNA framework and as a migration reference for
anyone porting XNA/MonoGame code to CNA.

**Current phase:** Phases 1 (Foundation), 2 (2D Games), and 5 (Audio) are complete except
for items deferred on missing CNA features. Phase 6 (Full games) is in progress
(GameStateManagement #072, CatapultWars #067, Yacht #071, SoccerPitch #073, HoneycombRush
#063 done; 8 of 14 still open, TankOnHeightmap #074 additionally deferred — same CNA gap
as SplitScreen, see below). Phase
7 (Advanced/UI/Misc) is well underway (GesturesSample #079, TouchThumbsticks #080,
LocalizationSample #078, SnowShovel #083, DynamicMenu #077, UISample #082,
PerformanceMeasuring #081 done — 7 of 9; the remaining 2 are both effectively deferred,
see below). NGSMSample #075 was investigated this session and found not viable as a
"quick next sample": ~1900 of its ~2900 lines (the whole `Networking/` folder — lobby,
session create/find/join, gamer profile sign-in) need `Microsoft.Xna.Framework.Net`/
`GamerServices`, which don't exist in CNA (same class of gap as PerformanceMeasuring's
omitted `RemoteDebugCommand`); the one path that doesn't need networking ("Single
Player") just loads an intentionally-empty `GameplayScreen` stub per the original's own
doc ("there is no actual game code included here"). User decided (this session) to mark
it Deferred in PLAN.md rather than port a misleadingly-thin single-player-only skeleton.
SplitScreen #076 was also investigated this session (see section 4D and
`samples/SplitScreen/missing.md`) and found genuinely blocked, but on a precisely
scoped, real CNA gap rather than the "3D shaders/models untouched" assumption from
before: it reuses `tank.fbx` (byte-identical to CameraShake's, `.model.json`
conversion already proven) and needs a settable `GraphicsDevice.Viewport` (already
supported) — the actual blocker is that `Tank.cs` animates the tank's wheels/turret/
cannon/hatch independently via named-bone lookups (`Model.Bones["l_back_wheel_geo"]`,
etc.), and CNA's `.model.json` reader currently only ever creates a single flat "Root"
bone with no per-mesh assignment (confirmed by reading `ContentManager.cpp`'s
`ModelTypeReader::Read()` and `ModelMesh`/`Model.cpp`). This is a small, well-scoped,
backward-compatible CNA fix (add a `ModelMesh` parent-bone setter + have the reader
create one bone per mesh) that the user will handle directly in the `cna` repo — not
attempted in this repo; `samples/SplitScreen/` currently has only `missing.md` and the
verbatim `.htm`, no source yet. DEFERRED.md item 6 was corrected to no longer claim
rigid multi-part bone animation is "just asset conversion" — it also needs this reader
change. Phases 3 (3D shaders) and 4 (models/animation) themselves are still untouched
as dedicated phases — no sample ported yet whose whole point is a custom HLSL→GLSL
effect or skeletal animation — but basic single-bone static-model loading is proven
twice (CameraShake, PerformanceMeasuring). All 11 then-open Phase 6 samples were also
surveyed this session for blockers before picking SoccerPitch: NetRumble (core Xbox
LIVE/System Link multiplayer + bloom shader, double-blocked), MarbleMaze and
CatapultWarsTrainingKit (multi-exercise training-kit structures, not one clean sample;
the latter is also just redundant re-implementations of the already-ported
CatapultWars), and RolePlayingGame (not technically blocked — 2D tile-based, no
models/shaders — but ~32,700 lines, a multi-session undertaking on its own) were set
aside. HoneycombRush #063 was ported this session (see section 3). NinjAcademy #065
and CardsStarterKit #069 were found to have no blockers and remain good candidates for
a future "next sample" pick.

**Key architectural decisions:**
- One executable per sample; no shared sample library. Each sample directory is
  self-contained (including its own UI-control-library code, even when two samples happen
  to port structurally similar libraries — see DynamicMenu vs. UISample in section 6).
- All sample code is header-only except each sample's `src/Program.cpp`.
- Assets: `Load<Texture2D>` → PNG; `Load<SpriteFont>` → `.font.json` + PNG atlas (via
  `tools/make_font.py`); `Load<SoundEffect>` → WAV; `Load<Song>` → OGG/WAV/MP3/FLAC. XNA
  `.xnb` is never supported and never will be. XML content-pipeline assets (custom
  `[ContentSerializer]` data, e.g. DynamicMenu's `MenuPage2.xml`) are hand-translated once
  into equivalent C++ construction code — there is no general XML deserializer, and writing
  one hasn't been justified by any single sample yet.
- `CONTENT_DIR` in each sample's `cna_add_sample()` call is **mandatory** for any sample
  that loads assets (including the F1 `help.png` overlay) — otherwise the binary aborts
  at startup.
- Default graphics backend is **EasyGL** (OpenGL ES); a Vulkan backend also exists but has
  a known bug (section 5).
- CNA is consumed via `add_subdirectory(../cna)`, so building any sample also rebuilds
  CNA (and transitively sharp-runtime) if their sources changed.
- `cna` and `sharp-runtime` are separate sibling git repositories, not submodules of this
  one. Framework-level bugs found while porting a sample get fixed in those repos
  directly (see section 3) — never worked around inside a sample. Confirm scope with the
  user before editing a shared framework repo; other work may be happening there
  concurrently.
- Touch/gesture-driven samples get a mouse fallback rather than being left touch-only,
  since this desktop has no touchscreen and CNA does not synthesize touch/gestures from
  mouse input. Three established patterns exist depending on the interaction shape — see
  section 6's "Touch/gesture input" note — pick whichever fits a new sample's mechanic
  rather than inventing a fourth.

---

## 2. Current status

### Build
40 enabled samples compile and link cleanly with the default **EasyGL** backend — a full
`cmake --build cmake-build-debug` (all targets, 0 errors) was run in the same session
HoneycombRush was added, so this is a live-confirmed guarantee, not just "known good as of
a commit." The active configured build tree is `cmake-build-debug`.

### Tests
No automated test suite exists in this repo. The samples themselves are the manual/visual
integration tests: verification is done by building a target, running it (optionally
under `SDL_VIDEODRIVER=x11` for headless screenshotting), and inspecting a screenshot or
driving it interactively with `xdotool`.

### CLI / tools available
- `tools/make_font.py` — generates a CNA `.font.json` + PNG glyph atlas from a TTF/TTC
  file and an explicit or default character set.
- `tools/gen_help_png.py` — extracts a sample's "Sample Controls" table from its `.htm`
  doc and renders the F1 help-overlay PNG; also exposes `build_text`/`render_png` as
  importable helpers for samples whose `.htm` lacks that table (see PerformanceMeasuring's
  missing.md for an example of authoring the control text manually with the same helpers).
- `tools/obj2model.py` — converts a Wavefront OBJ to CNA's `.model.json` +
  `_verts.bin`/`_idx.bin`. For FBX/`.x` source meshes, first run
  `assimp export source.fbx-or-.x out.obj`, then feed the resulting `.obj` to this script
  (established in CameraShake, reused as-is for PerformanceMeasuring's `Ground.x`).
- `tools/fbx_ascii2model.py` — an alternate FBX (ASCII 6.1) → `.model.json` converter,
  used for CameraShake's `tank.fbx` (mesh with many parts) instead of the assimp+obj2model
  route.
- No linter/formatter is configured in this repo.

### Recently implemented features
- HoneycombRush (#063): a full Phase 6 game port — a "collect honey from beehives while
  dodging bees, deposit it in the vat before time runs out" arcade game, from the
  `EX2_PolishAndMenus` training-kit stage. Largest port to date: ~30 source files
  (its own `ScreenManager`/`MenuScreen`/`MenuEntry`/`InputState` framework;
  `Misc/{Animation,AudioManager,ConfigurationManager,ExtensionMethods,
  VirtualThumbsticks}.hpp`; `Objects/{TexturedDrawableGameComponent,ScoreBar,Beehive,
  HoneyJar,Vat,SmokePuff,Bee,WorkerBee,SoldierBee,BeeKeeper}.hpp`; 7 `Screens/*.hpp`).
  Found and fixed **three real, reliably-reproduced C++-porting bugs** (not CNA bugs —
  see `missing.md` for full writeups):
  1. `AudioManager::LoadSound()` stored a `SoundEffectInstance` created from a *local*
     `SoundEffect` — CNA's `SoundEffectInstance` keeps a raw `const SoundEffect*` back
     to its source (unlike XNA's GC-backed reference), so every cached instance ended
     up dangling; crashed on the very first `PlaySound()` call in the whole sample.
  2. `BeeKeeper::Draw()` passed a temporary `Load<Texture2D>("Textures/hit")` straight
     into `SpriteBatch::Draw()`, whose deferred rendering defers the actual texture
     bind until `End()` — by which point the temporary was long gone; crashed the
     first time a bee stung the player.
  3. A same-frame reentrancy hazard in `ScreenManager`: `GameplayScreen` has
     `TransitionOffTime=0`, so leaving it (e.g. the pause menu's "Exit") synchronously
     destroys ~100 owned `Bee`/`Beehive`/`ScoreBar` `GameComponent`s *while*
     `Game::Update()` is still iterating a same-frame snapshot of `Game.Components`
     that still points at some of them — `pure virtual method called` /
     `SIGABRT`, reliably reproduced 2/2 attempts. Fixed by having `ScreenManager`
     defer the actual `shared_ptr` release to the start of the *next* `Update()` call
     (a point guaranteed to be outside any component-iteration snapshot). This class
     of bug (deterministic C++ destruction happening mid-iteration of something the
     GC-based original never would) is worth watching for in any future sample whose
     "screen" owns many `GameComponent`s directly.
  Other adaptations: `System.Threading.Thread`-based background loading became
  synchronous-with-a-one-frame-delay-flag (same pattern for both
  `LoadingAndInstructionScreen` and `LevelOverScreen`); `Guide.BeginShowKeyboardInput`
  (no CNA equivalent) became a fixed `"Player"` placeholder name;
  `IsolatedStorageFile` high-score persistence became plain file I/O;
  `AnimationsDefinition.xml`/`Configuration.xml` hand-translated into static C++
  tables (same "no general XML deserializer" precedent as DynamicMenu); a
  keyboard-driven virtual-thumbstick fallback (WASD/arrows, anchored at the on-screen
  joystick widget) stands in for the left touch since this desktop has no
  touchscreen, without needing any changes to the ported `BeeKeeper` direction logic.
  Interactively confirmed by screenshot end-to-end: title screen → Start →
  Instructions → tap-to-load → 3-second countdown → "Go!" → full gameplay (5
  beehives with swarming worker/soldier bees, honey jar fill UI, keyboard-driven
  beekeeper movement, Space-triggered smoke gun with a visible smoke-puff cloud, vat
  fill/timer UI) → Escape pause menu → "Exit" cleanly returns to the main menu with no
  crash (this exact interaction crashed reliably before bug #3's fix).
- SoccerPitch (#073): procedurally-generated 3D pitch demo showing
  `DualTextureEffect`/`AlphaTestEffect`/`BasicEffect` multipass rendering, a
  depth-biased flattened-sphere shadow, and a camera fly-over. `ProceduralPrimitive<T>`
  ported as a C++ template (only ever instantiated with the built-in
  `VertexPositionNormalTexture` — see missing.md for why the original's second,
  dual-UV vertex type isn't used). One real gap found and worked around with a
  documented visual simplification (user-approved): CNA's `DualTextureEffect` EasyGL
  shader has only one shared UV for both textures, not the original's two independent
  UV channels needed for base/detail textures to tile at different rates — the pitch
  now tiles both at the same rate instead of blocking on a CNA shader change. Also hit
  and fixed a real startup crash: `FrameRateCounter`'s own second `ContentManager`
  needs an explicit `setGraphicsDevice()` call before its first `Load<T>()` (XNA wires
  this automatically; CNA doesn't for a manually-constructed `ContentManager`).
  Interactively confirmed by screenshot: the dual-textured pitch, alpha-blend/
  alpha-test line-rendering toggle (via mouse click, since there's no touchscreen),
  the ball + shadow, camera fly-over, FPS counter, and F1 help overlay (hand-authored,
  same reason as PerformanceMeasuring — no "Sample Controls" table in this kit's
  `.htm`, which is also named `SoccerPitchOverview.htm` rather than the usual
  `SampleName.htm`).
- PerformanceMeasuring (#081): a full port of the GameDebugTools mini-library
  (DebugManager/DebugCommandUI/FpsCounter/TimeRuler/Layout/KeyboardUtils/
  IDebugCommandHost — `RemoteDebugCommand` omitted, Xbox 360 SystemLink-only, see
  missing.md) plus a 200-sphere bouncing/colliding 3D physics demo (spheres are a fresh
  copy of Primitives3D's flat-shaded `GeometricPrimitive`/`SpherePrimitive` per the
  "no shared library" rule) and a `Ground.x` model converted to `.model.json` via
  `assimp export` + `tools/obj2model.py` (same pipeline as CameraShake's tank/ground).
  Interactively confirmed by screenshot: FPS counter, TimeRuler profiling bars, sphere
  physics/collision toggle (`X`), the in-game debug console (`Tab` to open, typing +
  Backspace, `help` echoing all 5 registered commands including `TimeRuler`'s `tr` and
  `FpsCounter`'s `fps`), and the F1 help overlay (hand-authored, since this sample's
  `.htm` has no "Sample Controls" table to extract — see missing.md). `Tab` to close the
  console and `Up`/`Down` sphere-count adjustment were not independently re-confirmed —
  cut short when the shared desktop's real keyboard input momentarily crossed into the
  test window (see section 5's gotchas; not a code issue, both paths mirror
  already-confirmed logic).
- UISample (#082): builds on GameStateManagement, adding a second, independent UI control
  library (Control/TextControl/ImageControl/PanelControl/ScrollingPanelControl/
  PageFlipControl + ScrollTracker/PageFlipTracker) with Silverlight-Panorama-style page
  flipping (drag/flick through level-select pages) and Silverlight-ListBox-style scrolling
  (drag through a fake high-score list). All touch/gesture input (Tap for menu selection,
  HorizontalDrag/VerticalDrag/Flick/DragComplete for flip/scroll) is synthesized from mouse
  input in one place (`InputState::UpdateMouseFallback()`), so none of the ported control/
  tracker code needed CNA-specific changes. Found and fixed a real bug during interactive
  verification: `TouchPanel::DisplayWidth/Height` were never set, so
  `PageFlipTracker`'s drag-to-flip threshold computed against 0 (see missing.md).
- DynamicMenu (#077): a small reusable UI control library (Control/Container/Button/Label/
  TextControl/MultilineTextControl/Image/ProgressBar/PhoneScreen) plus a 3-page demo --
  Page 1's controls and Transition-based animations are laid out in code; Page 2/3's
  control trees, originally XML content-pipeline assets, are hand-built in C++ instead
  (no content pipeline in CNA -- see missing.md). Mouse-tap fallback (via a synthetic
  `GestureType::Tap`) and a keyboard orientation toggle (`O`) were added.
- SnowShovel (#083): portrait (480x800) accelerometer/tilt game — scoop snowflakes with a
  shovel before a shrinking-per-wave timer runs out; real accelerometer with
  gamepad/touch/keyboard fallback (same pattern as Bounce/Yacht); mouse click-and-drag
  fallback added after the user found the first pass had none (the original never used
  Mouse, only Touch/Gamepad/Keyboard) -- user-confirmed working on real hardware.
- LocalizationSample (#078): 5-language (en/da/fr/ja/ko) string + flag switching, SPACE
  cycles language.
- TouchThumbsticks (#080): twin-stick space shooter with real dual-touch virtual
  thumbsticks.
- GesturesSample (#079): real `TouchPanel` gesture recognition (Hold/Tap/Drag/Flick/Pinch).
- Two CNA framework fixes (asset path resolution; UTF-8 text decoding) — see section 3.
- A PLAN.md bookkeeping fix: Yacht (#071) has been done and committed since a prior
  session (commit `87f4111`) but its status row was never updated from "Todo" — fixed
  this session while reconciling PLAN.md's done-count against the actual built binaries.

### Known working examples / demos
40 samples run end-to-end; see the full table in `PLAN.md` or run
`cmake --build cmake-build-debug` and look under `samples/*/` for the current set.
Representative, recently-verified ones:
- `HoneycombRush_cna_samples` — title → Start → Instructions → tap-to-load →
  countdown → full gameplay (beehives/bees/honey jar/vat/smoke gun, keyboard-driven
  movement) → Escape pause menu → "Exit" back to main menu, all confirmed by
  screenshot with no crash. See "Recently implemented features" above and
  `samples/HoneycombRush/missing.md` for the three real bugs found and fixed.
- `SoccerPitch_cna_samples` — dual-textured pitch (single shared UV, see missing.md),
  alpha-blend/alpha-test line toggle (mouse click), ball + depth-biased shadow, camera
  fly-over, FPS counter, and F1 help overlay all confirmed by screenshot.
- `PerformanceMeasuring_cna_samples` — FPS counter, TimeRuler bars, 200-sphere
  bounce/collide physics, collision toggle (X), debug console (Tab/typing/Backspace/
  `help`), and F1 help overlay all confirmed by screenshot. `Tab`-to-close and
  `Up`/`Down` sphere count not independently re-confirmed — see that sample's
  `missing.md`.
- `UISample_cna_samples` — Main Menu, mouse-tap navigation into Level Select via
  LoadingScreen, and mouse-drag page-flipping (House → Pasture) all confirmed by
  screenshot. HighScoreScreen's vertical scroll could not be drag-tested via `xdotool`
  on this desktop (mouse position freezes while a button is held — see that sample's
  `missing.md` and section 4C); code-reviewed with no bug found.
- `DynamicMenu_cna_samples` — Page 1, Page 2 (UFO image + multiline text), Page 3
  (progress bar advances correctly via the `Advance` button), and the `O` orientation
  toggle (round-tripped portrait↔landscape twice, no crash) all confirmed by screenshot
  across two sessions. A static line-by-line correctness review separately caught and
  fixed one real bug (a transition-list reentrancy issue -- see that sample's
  `missing.md`).
- `SnowShovel_cna_samples` — pre-game screen, SPACE-to-start, scoring, countdown timer
  color-shift, automatic game-over transition, and the F1 help overlay all confirmed by
  screenshot; mouse click-and-drag movement and click-to-start/restart confirmed working
  by the user directly on real hardware.
- `GesturesSample_cna_samples` — touch/mouse sprite manipulation.
- `TouchThumbsticks_cna_samples` — twin-stick shooter, keyboard+mouse fallback.
- `LocalizationSample_cna_samples` — language cycling with SPACE, all 5 languages render
  correctly including Japanese/Korean.
- `Yacht_cna_samples`, `CatapultWars_cna_samples`, `GameStateManagement_cna_samples` —
  full games with menu/screen frameworks.

### What does not work yet
- **CameraShake** 3D scene renders as a white stripe on all backends (see section 4A).
- **Phases 3 & 4** (3D shaders / models + animation): no sample ported yet; no
  HLSL→GLSL shader workflow or model-conversion pipeline exists in this repo.
- **Vulkan backend**: a second `SpriteBatch.Begin/End` in the same frame discards the
  first (EasyGL, the default backend, does not have this problem).
- 4 samples (BloomSample, ColorReplacement, ReachGraphicsDemo, Spacewar) have source
  ported from earlier work but no `CMakeLists.txt` yet — not buildable as-is.
- DynamicMenu Page 2/3 and UISample's HighScoreScreen scroll: implemented and believed
  correct, but not independently re-confirmed by screenshot this session (see above).

---

## 3. Recent changes

Most recent first, matching `git log`:

- **(uncommitted)** — Added `samples/HoneycombRush/` (#063): the largest port to date
  (~30 source files) — a "collect honey while dodging bees" arcade game with its own
  `ScreenManager` framework, `Misc/` helpers, 10 `Objects/*.hpp` gameplay entities, and
  7 `Screens/*.hpp`. Found and fixed three real, reliably-reproduced C++-porting bugs
  (not CNA bugs — full writeups in `missing.md`): (1) `AudioManager::LoadSound()`
  cached a `SoundEffectInstance` whose source `SoundEffect` was a local temporary —
  CNA's `SoundEffectInstance` holds a raw pointer back to it (unlike XNA's GC-backed
  reference), so it dangled and crashed on the first `PlaySound()` call in the whole
  sample; fixed by also caching the `SoundEffect`s themselves. (2) `BeeKeeper::Draw()`
  passed a temporary `Load<Texture2D>("Textures/hit")` directly into
  `SpriteBatch::Draw()`, whose deferred rendering doesn't touch the texture until
  `End()` — by then the temporary was gone; fixed by caching the texture in
  `LoadContent()`. (3) A same-frame reentrancy hazard in `ScreenManager`:
  `GameplayScreen` (`TransitionOffTime=0`) synchronously destroys ~100 owned
  `GameComponent`s when exited (e.g. via the pause menu's "Exit"), while
  `Game::Update()` is still iterating a same-frame snapshot of `Game.Components` that
  still points at some of them — `pure virtual method called`, reliably reproduced
  2/2. Fixed by having `ScreenManager::RemoveScreen()` defer the screen's actual
  `shared_ptr` release to the start of the *next* frame's `Update()` (guaranteed
  outside any component-iteration snapshot) instead of letting it drop immediately.
  This class of bug — deterministic C++ destruction happening mid-iteration of
  something the GC-based C# original never would — is worth watching for in any
  future sample whose "screen" owns many `GameComponent`s directly. Other
  adaptations: background-thread asset loading → synchronous with a one-frame-delay
  flag; `Guide.BeginShowKeyboardInput` (no CNA equivalent) → fixed `"Player"` name;
  `IsolatedStorageFile` → plain file I/O; `AnimationsDefinition.xml`/
  `Configuration.xml` hand-translated into static tables; keyboard (WASD/arrows)
  fallback for the left virtual thumbstick, anchored at the on-screen joystick
  widget's center, needing zero changes to the ported `BeeKeeper` direction logic.
  Interactively confirmed end-to-end by screenshot: title → Start → Instructions →
  tap-to-load → countdown → "Go!" → full gameplay (5 beehives, swarming bees, honey
  jar UI, keyboard movement, Space-triggered smoke with visible smoke-puff cloud, vat
  fill/timer UI) → Escape pause menu → "Exit" cleanly back to the main menu (the exact
  interaction that crashed reliably before bug #3's fix).
- **(uncommitted)** — Added `samples/SoccerPitch/` (#073): procedurally-generated 3D
  pitch demo (`DualTextureEffect`/`AlphaTestEffect`/`BasicEffect` multipass rendering,
  depth-biased shadow, camera fly-over). `ProceduralPrimitive<T>` ported as a C++
  template, only ever instantiated with the built-in `VertexPositionNormalTexture`.
  Found a real, narrow CNA gap: `DualTextureEffect`'s EasyGL shader has one shared UV
  for both textures, not the original's two independent UV channels for
  differently-tiled base/detail textures — ported with a user-approved, documented
  visual simplification (single shared tiling factor) instead of blocking on a CNA
  shader change. Also found and fixed a real startup crash: `FrameRateCounter`'s own
  second `ContentManager` needs an explicit `setGraphicsDevice()` call before its
  first `Load<T>()`, which XNA wires automatically but CNA doesn't for a
  manually-constructed `ContentManager`. This kit's `.htm` is named
  `SoccerPitchOverview.htm` (not `SampleName.htm`) and has no "Sample Controls" table,
  so F1's text was hand-authored, same approach as PerformanceMeasuring. Interactively
  confirmed by screenshot: the pitch, alpha-blend/alpha-test toggle (mouse click), ball
  + shadow, camera fly-over, FPS counter, and F1. Also surveyed all 11 then-open
  Phase 6 samples for blockers before picking this one — see section 1.
- **`7b36712`**, **`5d4ff9e`**, **`f1c2874`** — Investigated NGSMSample (#075) and
  SplitScreen (#076); both marked Deferred in PLAN.md (also TankOnHeightmap #074, same
  root cause as SplitScreen) rather than ported thin/blocked — see section 1 for the
  detailed reasoning and `samples/SplitScreen/missing.md` for the CNA-side technical
  write-up (per-mesh `ModelBone` support, to be implemented by the user directly in
  `cna`, not attempted in this repo).
- **`fa07b7a`** — Added `samples/PerformanceMeasuring/` (#081): a full port of the
  GameDebugTools mini-library (`DebugManager`/`DebugCommandUI`/`FpsCounter`/`TimeRuler`/
  `Layout`/`KeyboardUtils`/`IDebugCommandHost` — `RemoteDebugCommand` omitted, Xbox 360
  SystemLink-only, no CNA equivalent, see missing.md) plus a 200-sphere bouncing/
  colliding 3D physics demo instrumented with `TimeRuler.BeginMark`/`EndMark` around
  `Update`/`Draw`. `Primitives/GeometricPrimitive.hpp`/`SpherePrimitive.hpp` are a fresh
  copy of Primitives3D's flat-shaded versions (per "no shared sample library"; CNA still
  has no `VertexPositionNormal`, so spheres are flat-colored, not lit — DEFERRED.md item
  5). `Ground.x` was converted via `assimp export Ground.x ground.obj` then
  `tools/obj2model.py` — the exact same two-step pipeline already proven in CameraShake,
  reused here for the first time by a *second* sample, confirming it's a repeatable
  workflow, not a one-off. This sample's `.htm` has no "Sample Controls" table (unlike
  every other sample so far), so the F1 help overlay's text was hand-authored by
  importing `gen_help_png.py`'s own `build_text`/`render_png` helpers rather than
  extracting from the doc — see missing.md. Interactively confirmed by screenshot: FPS
  counter, TimeRuler bars, sphere physics/collisions, the `X` collision toggle, the
  debug console (`Tab` open, typing, `Backspace`, `help` executing and echoing all 5
  registered commands), and F1. `Tab`-to-close and `Up`/`Down` sphere-count were not
  re-confirmed — testing was cut short when a screenshot showed text in the console this
  session never typed, i.e. real keyboard input from the desktop's actual user crossed
  into the test window (the reverse direction of the already-documented shared-desktop
  focus flakiness — see section 5's new gotcha entry).
- **`3526b60`** — `NEXT.md`/`DynamicMenu/missing.md`/`UISample/missing.md`: recorded
  interactive re-verification of DynamicMenu (Page 2/3, progress bar, `O` toggle — all
  confirmed working, no bugs) and UISample (HighScoreScreen scroll could not be
  drag-tested — `xdotool`'s mouse-button-held motion freezes to the game's polled
  `Mouse::GetState()` on this desktop even though the real X11 pointer keeps moving;
  code-reviewed with no bug found, likely an X11 grab quirk of this desktop, not CNA).
- **`579a7c8`** — Added `samples/UISample/` (#082): a second, independent UI control
  library (`Controls/{Control,TextControl,ImageControl,PanelControl,ScrollingPanelControl,
  PageFlipControl,ScrollTracker,PageFlipTracker,HighScorePanel}.hpp`) plus the
  GameStateManagement-derived screen framework (`GameScreen.hpp`, `ScreenManager.hpp`,
  `MenuScreen.hpp`, `MenuEntry.hpp`, `InputState.hpp`, `Screens.hpp`), extended with
  per-screen `EnabledGestures` routing (not present in the existing GameStateManagement
  port, which doesn't need touch). All touch/gesture input synthesized from mouse in one
  place, `InputState::UpdateMouseFallback()` -- see missing.md. Found and fixed a real bug
  during interactive verification: `TouchPanel::DisplayWidth`/`Height` were never set in
  `UISampleGame::Initialize()`, so `PageFlipTracker`'s drag-to-flip threshold computed
  against 0 and no drag could ever cross it. Also fixed, in `DynamicMenu/Control.hpp`: a
  wrong claim (made in this session's own chat, not previously written into any file) that
  CNA's `SpriteBatch::Draw` has no destRect+rotation+origin+effects+depth overload -- it
  does (the user caught this); the actual bug was passing `std::nullopt` to a parameter
  typed `const Rectangle&`, not `std::optional<Rectangle>`. New files:
  `samples/UISample/{CMakeLists.txt,missing.md,UISample.htm,
  src/{Program.cpp,UISampleGame.hpp,CommonGraphics.hpp,GameScreen.hpp,ScreenManager.hpp,
  MenuScreen.hpp,MenuEntry.hpp,InputState.hpp,Screens.hpp,Controls/*.hpp},Content/...}`.
- **`1ebeff3`** — Added `samples/DynamicMenu/` (#077): a reusable UI control library
  (`Controls/{Control,Container,Button,Label,TextControl,MultilineTextControl,Image,
  ProgressBar,PhoneScreen}.hpp`, `Transitions/Transition.hpp`) plus a 3-page demo.
  Page 2/3's control trees were originally XML content-pipeline assets; hand-built as
  equivalent C++ construction (`BuildMenuPage2Content()`/`BuildMenuPage3Content()` in
  `DynamicMenuGame.hpp`) since CNA has no content pipeline -- see missing.md. Added a mouse
  fallback (synthesizes a `GestureType::Tap` on click, so every button gets it for free
  with no changes to the ported library) and a keyboard (`O`) orientation toggle standing
  in for real device rotation. Caught and fixed one real bug via static review (not live
  testing): a naive `std::vector<Transition>` (by value) in `Control::activeTransitions_`
  would corrupt itself if a `TransitionComplete` callback applied a new transition
  mid-iteration (the Get-Big button does exactly this) -- fixed by switching to
  `std::vector<std::shared_ptr<Transition>>`, matching the original's C# reference-type
  semantics.
- **`f6b3650`** — Added `samples/SnowShovel/` (#083): portrait accelerometer/tilt game,
  real `Microsoft::Devices::Sensors::Accelerometer` with gamepad/touch/keyboard fallback
  (`Accelerometer.hpp`, same pattern as Bounce/Yacht), plus a mouse click-and-drag fallback
  added after the user found the first pass didn't respond to the mouse at all (confirmed
  working afterward, on real hardware). Found and worked around (at the sample level, not
  in CNA) a viewport-timing quirk: `getViewportProperty()` returns a stale/wrong size when
  queried from `Initialize()`, even though the SDL window is already correctly sized by
  then — see `samples/SnowShovel/missing.md` for the full writeup and section 5/6 below.
- **`d90ec24`** — Added `samples/LocalizationSample/` (#078): English/Danish/French/
  Japanese/Korean welcome text + flag, switched via a `LoadLocalizedAsset<T>` fallback
  (try full culture name, then language-only, then default). sharp-runtime's
  `CultureInfo.CurrentCulture` is a stub (always invariant, no real OS locale detection),
  so SPACE manually cycles the 5 languages instead.
- **CNA repo `80757b1`** — Fixed `ContentManager::ResolveAssetPath`
  (`cna/include/Microsoft/Xna/Framework/Content/ContentManager.hpp`): it used
  `std::filesystem::path::has_extension()` to decide whether to append a reader
  extension, which misfires on any asset name containing a non-extension dot (e.g.
  `"Flag.en-US"` was mistaken for already ending in a `.en-US` extension, so `.png` was
  never appended). Now checks literal-path existence first. Found while porting
  LocalizationSample.
- **CNA repo `41a4766`** — Fixed `SpriteFont::MeasureString` and
  `SpriteBatch::DrawString` (`cna/src/Microsoft/Xna/Framework/Graphics/{SpriteFont,
  SpriteBatch}.cpp`): both iterated the input string one raw `char` (byte) at a time
  with no UTF-8 decoding, so any non-ASCII character (accented Latin, Japanese, Korean,
  anything outside ASCII) split into 2-4 bytes that each missed the glyph lookup and
  rendered as `?`. Added `CNA::Internal::DecodeUtf8CodePoint()`
  (`cna/include/CNA/Internal/Utf8Decode.hpp`) and used it in both places. Verified
  against a full rebuild of all 33 prior samples plus a text-heavy ASCII sample
  (InputReporter) with no regressions.
- **`6a24ffb`** — Added `samples/TouchThumbsticks/` (#080): real dual-touch
  `VirtualThumbsticks` (ported faithfully from the original), plus a WASD (movement) +
  mouse-position-relative-to-center (aim + auto-fire) keyboard/mouse fallback, since two
  simultaneous contacts can't be emulated with one mouse pointer.
- **`0a2b306`** — Added `samples/GesturesSample/` (#079): real `TouchPanel` gesture
  recognition (Hold/Tap/DoubleTap/FreeDrag/Flick/Pinch) manipulating a cat sprite, plus a
  parallel mouse fallback (drag/click/hold-timer/scroll-wheel) for desktop testing.
- Earlier history (Yacht #071 real touch/accelerometer, CatapultWars #067,
  GameStateManagement #072, and the Phase 1/2/5 foundational samples) — see `git log`
  and `PLAN.md` for the full list; not repeated here to keep this section current.

---

## 4. Current blocker / main problem

There is **no single hard blocker** stopping all progress — new portable 2D/Phase 7
samples can still be ported freely. The most significant *open* problems are:

### A. CameraShake 3D white stripe (confirmed bug, all backends)
- **Symptom:** The 3D scene renders as a thin bright bar; the tank and ground are
  invisible.
- **Failing command:**
  `./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples`
- **Affected:** `samples/CameraShake/src/CameraShakeGame.hpp` (camera at
  (1000,1000,1000), ground scale 0.1 → vertices at ±6554); CNA's near-plane clipping of
  w<0 vertices in the EasyGL/Vulkan backends.
- **Suspected cause:** CNA does not clip primitives against the near plane the way
  DirectX does, so vertices with w<0 project incorrectly.
- **Already tried:** Removed an incorrect z-remap from the Vulkan 3D vertex shaders
  (correct fix, but no visual change); a scale-0.02 workaround in the sample was
  rejected (don't hide a framework bug in a sample); EasyGL shows the identical stripe,
  confirming it's not backend-specific. The real fix belongs in CNA's shared clipping
  logic (not yet located/attempted).

### B. Phase 3/4 asset-pipeline gap (capability gap, blocks ~28 samples)
- Phase 3 needs custom HLSL effects translated to GLSL `.shader.json`; Phase 4 needs
  model conversion to `.model.json`, plus skeletal animation for several samples. No
  proven workflow for either exists in this repo yet — this is a capability gap, not a
  bug to fix.

### C. UISample's HighScoreScreen scroll — cannot be synthetically drag-tested on this desktop (minor, not blocking)
- **(resolved this session)** DynamicMenu's Page 2/3, the progress bar (`Advance` button),
  and the `O` orientation toggle were all interactively re-verified by screenshot — see
  section 3. No bugs found; all work as coded.
- UISample's HighScoreScreen (`ScrollTracker` vertical drag) was code-reviewed line by
  line (`ScrollTracker.hpp`, `ScrollingPanelControl.hpp`, `HighScorePanel.hpp`,
  `InputState::UpdateMouseFallback()`) and no logic bug was found, but it could **not** be
  interactively confirmed: while a mouse button is held down via `xdotool
  mousedown`/`mousemove`/`mouseup`, the game's polled `Mouse::GetState()` position freezes
  at the press location for the entire hold — confirmed with temporary debug
  instrumentation that `xdotool getmouselocation` (the real X11 pointer) keeps updating
  live while the in-game position does not move until the button is released, at which
  point it immediately jumps to the correct final position. Mouse motion **without** a
  button held updates every frame with no issue (confirmed by the same instrumentation).
  This is very likely an X11 pointer-grab interaction specific to this desktop's
  WM/compositor setup (see the `mutter-x11-frames`/focus-flakiness gotchas in section 5),
  not a CNA or sample bug — SnowShovel's analogous mouse-drag fallback was already
  confirmed working end-to-end on real hardware in an earlier session. Re-attempt this
  verification with a real mouse/touchscreen rather than `xdotool` if it needs a
  definitive answer; do not "fix" `InputState`/`ScrollTracker` based on this alone.

### D. SplitScreen (#076) blocked on CNA `.model.json` per-mesh bone support (capability gap, not attempted here)
- Investigated this session; not a port attempt, no `samples/SplitScreen/src/` exists
  yet — only `missing.md` and a verbatim copy of the `.htm`. Full technical write-up in
  `samples/SplitScreen/missing.md`; the short version: `Tank.cs` needs
  `Model.Bones["l_back_wheel_geo"]`-style named-bone lookups to animate the tank's
  wheels/turret/cannon/hatch independently, but CNA's `.model.json` reader
  (`ContentManager.cpp`'s `ModelTypeReader::Read()`) only ever creates one flat "Root"
  bone — every mesh's parent bone is null, and `ModelMesh` has no setter to give it one.
  Everything else the sample needs (settable `GraphicsDevice.Viewport`, `SpriteBatch`
  divider lines, the `tank.fbx`→`.model.json` conversion itself) is already proven/fine.
  User decided to fix this directly in the `cna` repo themselves rather than have it
  done here — do not attempt the CNA-side fix in this repo unless asked again.
  DEFERRED.md item 6 was corrected accordingly (it previously overclaimed that rigid
  multi-part bone models were "just asset conversion, no CNA code changes needed").

---

## 5. Known bugs and limitations

| Status | Description |
|--------|-------------|
| confirmed bug | **CameraShake white stripe (all backends).** See section 4A. |
| confirmed bug | **Multiple SpriteBatch/frame on Vulkan.** A second `Begin/End` in the same frame discards the first. Works fine on EasyGL (the default). Affects multi-layer samples (e.g. GameStateManagement, CatapultWars) only if run on the Vulkan backend. |
| incomplete | **Phase 3 (3D shaders)** — 19 samples blocked on HLSL→GLSL translation, no workflow exists. |
| incomplete | **Phase 4 (Models/Anim)** — 9 samples blocked on a model-conversion + animation pipeline, no workflow exists. |
| incomplete | **4 samples have source but no `CMakeLists.txt`** (BloomSample, ColorReplacement, ReachGraphicsDemo, Spacewar); their `help.png` was generated without a `CONTENT_DIR` wired up — add both when enabling them. |
| limitation | **No SpriteFont `.xnb` pipeline.** Atlases must be generated with `tools/make_font.py`; the original samples' actual font families (e.g. "Segoe UI", "Moire ExtraBold") are substituted with DejaVu (Latin) or Noto Sans CJK (when a sample needs non-Latin text). |
| limitation | **`sharp-runtime`'s `CultureInfo.CurrentCulture` is a stub** — always returns the invariant culture; there is no real OS locale detection. LocalizationSample works around this by manual language cycling (SPACE key) instead of auto-detecting the OS locale. |
| limitation | **No content-pipeline XML deserializer.** DynamicMenu's Page 2/3 were originally XML content-pipeline assets; hand-translated once into equivalent C++ construction instead (see section 6). Fine for one-off small pages; would need a real solution if a future sample has a large or frequently-edited XML-driven layout. |
| needs verification | **Real hardware accelerometer** — Yacht's shake-to-roll and SnowShovel's tilt are only verified via the keyboard/gamepad/touch fallback on this desktop (no physical accelerometer available). The real-sensor code path (`Accelerometer::getIsSupportedProperty()` true branch) is implemented but untested end-to-end on real hardware. |
| fixed (verified, no bug) | **DynamicMenu Page 2/3, the progress bar, and the `O` orientation toggle.** Interactively re-verified this session: Page 2 (UFO image + multiline text), Page 3 (`Advance` button increments the progress bar's arrow fill correctly, 0→100 in steps of 10), and `O` (portrait↔landscape, round-tripped twice with no crash) all work as coded. See section 4A/3. |
| environment limitation | **UISample's HighScoreScreen (`ScrollTracker` vertical scroll) cannot be drag-tested via `xdotool` on this desktop.** Code-reviewed with no bug found; interactive drag confirmed blocked by the game's `Mouse::GetState()` freezing position while a button is held (real X11 pointer keeps moving per `xdotool getmouselocation`). See section 4C for the full writeup — likely an X11 grab quirk of this desktop, not a CNA/sample bug. |
| needs verification | **`GraphicsDevice.Viewport` is stale when queried from `Game::Initialize()`.** Observed while porting SnowShovel: `getViewportProperty().getWidthProperty()/getHeightProperty()` returned `1333x800` instead of the requested `480x800` even though the SDL window was already correctly sized (confirmed via `xwininfo`) and `GraphicsDeviceManager::CreateDevice()` runs before `Initialize()` per `Game::DoInitialize()`. Worked around in each affected sample (use the known preferred-back-buffer constants instead of re-querying); not root-caused inside CNA. Existing samples that call `getViewportProperty()` all do so later, from `Update()`/`Draw()` (e.g. Yacht's `ScreenManager::SafeArea()`), so this may be a wider latent issue worth a real CNA-side fix if a future sample needs viewport sizing genuinely inside `Initialize()`. `TouchPanel::DisplayWidth`/`Height` has the same shape of gotcha (see UISample's entry below) but is a distinct property with its own explicit setter, not the same underlying bug. |
| gotcha | **`xdotool` input can silently no-op if the target window lost X focus** (e.g. after an intervening screenshot/tool call) — clicks/keys are sent but never reach the app, with no error message. Always run `xdotool windowactivate --sync $WID && xdotool windowfocus --sync $WID` immediately before each simulated input burst. A plain `xdotool click`/`key` can also complete faster than one game-loop frame (~33ms at the sample's 30fps) and be missed entirely by a naive down/up edge check — prefer explicit `keydown`/`sleep 0.15`/`keyup` over `click`/`key` when the app must observe a discrete press. |
| gotcha | **This is a shared, actively-used desktop session, not an isolated headless sandbox.** `xdotool getactivewindow` has returned an unrelated window (e.g. a git history browser the human user had open) instead of the sample under test, and `windowactivate`/`windowfocus` on the sample's window has repeatedly failed to take real effect (confirmed via a 1x1 `mutter-x11-frames` proxy window absorbing X input focus) even though `getactivewindow` claimed success right after the call — sometimes `windowactivate --sync` followed immediately by `getactivewindow` still reports the proxy window, meaning the activate call itself silently failed, not just "focus was lost since the last check." Always re-verify with `getactivewindow` **immediately before** every input burst, not just once at the start of a test sequence. If input stops reaching a sample mid-session for no code-related reason, check `xdotool getactivewindow` + `getwindowname`/`xwininfo -tree` for what's *actually* focused before assuming a CNA/sample bug — and stop sending synthetic keys/clicks rather than risk them landing on the user's own foreground application. |
| gotcha | **`xdotool mousemove` while a button is held down does not appear to reach a CNA/SDL window's polled mouse state on this desktop**, even though the real X11 pointer position does move (`xdotool getmouselocation` updates live). The frozen position snaps to the correct final value the instant the button is released. This blocks synthetic drag-testing (scroll/page-flip/drag mechanics) via `xdotool` specifically during a held button; motion with no button held is unaffected. Confirmed via temporary `fprintf` instrumentation in `UISample/src/InputState.hpp` (see section 4C) — do not re-add that debug print without removing it again afterward. Likely an X11 grab handoff quirk of this desktop's WM, not a CNA bug (SnowShovel's mouse-drag fallback was already confirmed on real hardware). If a future session needs a definitive drag-mechanic verification, use real hardware, not `xdotool`. |
| gotcha | **Input can cross in the *other* direction too: the real user's own keystrokes can land in a test window instead of the app they were actually typing into.** While testing PerformanceMeasuring's debug console, a screenshot showed text (`"T zadne_"`) in the console's command line that this session never typed — almost certainly a fragment of the desktop's real user typing something else at the moment focus happened to be on the test window. Confirmed with the user that nothing of theirs appeared to be affected, but treat this as seriously as the reverse case: if unexplained text/input appears in a test window, stop immediately, kill the test process to release focus, and ask the user to check whatever they were doing at the time before resuming. |
| gotcha | **A multi-line `xdotool ...` shell block can get concatenated into one chained invocation** (xdotool supports `mousemove X Y mousedown 1 sleep 0.1 mousemove ...` as a single chained command). A shell block with several intended-to-be-sequential lines (`xdotool mousemove ...`, `sleep`, `xdotool mousedown 1`, ...) got joined into arguments of a single `xdotool` call, which then ran detached in the background indefinitely, replaying/holding mouse state and flooding an app with far more input than intended — eventually crashing it. This was a test-harness mistake, not an app bug. Always issue one xdotool action per shell invocation (or join intentionally with `;`/`&&`), and check `jobs -l`/`ps aux \| grep xdotool` if a sample behaves as though it's receiving phantom input. |
| fixed (in `cna`, not this repo) | `ContentManager::ResolveAssetPath` misresolved asset names with a non-extension dot (e.g. `"Flag.en-US"`). See section 3, commit `80757b1` in the `cna` repo. |
| fixed (in `cna`, not this repo) | `SpriteFont`/`SpriteBatch::DrawString` had no UTF-8 decoding, so any non-ASCII text rendered as `?`. See section 3, commit `41a4766` in the `cna` repo. |
| pattern to watch for | **A `shared_ptr`-owning "screen"/"parent" object with `TransitionOffTime=0` (or any other synchronous-removal path) that itself owns many `GameComponent`s can crash with `pure virtual method called`/`SIGABRT` if leaving it is triggered from inside `Game::Update()`'s own component iteration.** `Game::Update()`/`Draw()` iterate a snapshot of `Game.Components` taken at the top of the frame; if a same-frame chain of calls destroys the owning object (and cascades into destroying its owned components) before that snapshot's iteration finishes, the remaining snapshot entries are dangling. XNA's GC never frees anything synchronously mid-iteration, so the original C# has no equivalent hazard. Found and fixed in HoneycombRush's `ScreenManager` (deferred the actual `shared_ptr` release to the start of the next frame's `Update()` — see section 3) — check for the same shape of bug in any future sample where a screen/parent directly owns and registers many `GameComponent`s. |
| pattern to watch for | **A temporary object (e.g. `Content.Load<T>()`'s return value) passed directly into `SpriteBatch::Draw()` can dangle.** `SpriteBatch`'s default (deferred) sort mode stores a raw pointer to the argument and doesn't actually use it until `End()` — if the argument was a temporary, it's already destroyed by then. Found and fixed twice in HoneycombRush (`AudioManager`'s cached `SoundEffectInstance`, and `BeeKeeper::Draw()`'s `"hit"` texture — see section 3/missing.md); always load into a stable member/cache first, never pass a `Load<T>()` call's return value straight into `Draw()`/anything that stores a reference for later use. |
| fixed (documentation only) | A claim made in chat this session that CNA's `SpriteBatch::Draw` has no destRect+rotation+origin+effects+depth overload was **wrong** — it exists (`SpriteBatch.hpp` around line 185), the user caught it. The actual bug was in `DynamicMenu/Control.hpp`: passing `std::nullopt` to a parameter typed `const Rectangle&` (not `std::optional<Rectangle>`). Before citing a CNA API "gap," grep the actual header first. |

---

## 6. Architecture notes

### Sample directory layout
```
samples/SampleName/
  src/Program.cpp        # int main(){ NS::GameClass g; g.Run(); return 0; }
  src/*.hpp              # all game logic, header-only
  Content/               # copied next to the binary via CONTENT_DIR (POST_BUILD)
  CMakeLists.txt         # cna_add_sample(name SOURCES src/Program.cpp CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Content)
  missing.md             # required: documents every difference vs the XNA original
  SampleName.htm         # required: original MS documentation (authored if the kit lacks one)
```

### CNA API rules — boundaries that must not be broken
- All XNA properties use `getXxxProperty()` / `setXxxProperty()` — never raw member
  access on framework types. (Plain hand-written sample classes, e.g. `Sprite`, `Ship`,
  DynamicMenu's/UISample's own `Control`, use ordinary `getFoo()`/`setFoo()` without the
  "Property" suffix — that suffix is reserved for real CNA/XNA framework types generated
  via the `DEF_PROP` macro.)
- Every `Game` **and** `DrawableGameComponent` subclass must implement
  `const std::string& GetTypeName() const override`.
- `Content.Load<T>()` returns **T by value** — store results in `std::optional<T>` (or a
  plain member with a no-arg default constructor, since `Texture2D` etc. share GPU
  backends via an internal `shared_ptr` and are cheap to copy). Anything that keeps a raw
  pointer *into* such a value (e.g. a sprite-sheet `Animation` referencing a shared
  texture, or DynamicMenu/UISample's `TextControl` pointing at a shared `SpriteFont`) must
  ensure that value lives at a **stable address** (`std::optional`/`std::map` node, member
  field constructed via an initializer list — never a reallocating `std::vector`).
- Virtual overrides on `Game`: `void Update(GameTime&)` (non-const) and
  `void Draw(const GameTime&)` (const). `DrawableGameComponent::Initialize()` calls
  `LoadContent()` once.
- No `operator+=` for `Vector2`/`TimeSpan` — write `v = v + expr`. `Color` has no
  `operator*` by float scalar built into every constructor form, but `SpriteBatch.hpp`
  **does** have the full destRect+source+color+rotation+origin+effects+depth `Draw`
  overload — verify against the actual header before assuming a `SpriteBatch::Draw`
  overload is "missing" (see section 5's last row).
- `Rectangle` exposes public `X/Y/Width/Height` and `Contains(x, y)`/`Contains(Point)` (no
  `getXProperty`), plus read-only `getLeftProperty()`/`getTopProperty()`/
  `getRightProperty()`/`getBottomProperty()`/`getCenterProperty()`. `Point`/`Vector2`
  expose public `X/Y` directly.
- `CONTENT_DIR` must be `${CMAKE_CURRENT_SOURCE_DIR}/Content` (the POST_BUILD copy runs
  from the binary dir, so a bare relative path fails silently at build time and loudly
  at runtime).
- SpriteFont atlases must be generated with `--content-root samples/X/Content` so the
  generated `.font.json`'s `texture` path resolves relative to that sample's Content
  root.
- `SpriteFont`/`SpriteBatch::DrawString` now correctly decode UTF-8 (as of CNA commit
  `41a4766`) — do not reintroduce a raw-byte `for (char c : text)` loop over a
  `std::string` when touching this code; use
  `CNA::Internal::DecodeUtf8CodePoint()` from `CNA/Internal/Utf8Decode.hpp`.
- `ContentManager::ResolveAssetPath` now resolves by literal-path existence, not
  `std::filesystem::path::has_extension()` (CNA commit `80757b1`) — this is what makes
  culture-suffixed asset names like `"Flag.en-US"` resolve correctly; don't revert to
  the extension-heuristic approach.
- A sample that queries `GraphicsDevice.Viewport` or `TouchPanel::DisplayWidth`/
  `DisplayHeight` **inside `Initialize()`** will get stale/zero values — both must either
  be set explicitly with known constants at that point, or read later from `Update()`/
  `Draw()` instead. See the two "needs verification" rows in section 5 for the exact
  symptoms each produced.

### ScreenManager framework (shared shape of GameStateManagement, CatapultWars, UISample)
- `Game` owns a `ScreenManager` (a `DrawableGameComponent`, added to `Components`).
- `ScreenManager` holds a stack of `std::shared_ptr<GameScreen>`; `Update` copies the
  stack, routes input to the topmost active screen, and lets each screen transition.
- **Invariant:** a screen may remove itself during its own `Update` (via `ExitScreen` →
  `RemoveScreen(this)`); the `shared_ptr` copy taken at the start of the update loop
  keeps it alive until the iteration ends. Do **not** switch screen ownership to raw
  owning pointers — that reintroduces use-after-free.
- Cross-referencing screens (screen A creates screen B which creates A) are resolved
  with forward declarations + out-of-line inline definitions at the bottom of
  `Screens.hpp`.
- UISample extends this shared shape with per-screen `GameScreen::EnabledGestures()`/
  `setEnabledGestures()` and `InputState::TouchState`/`Gestures` (see below) — neither
  exists in the GameStateManagement/CatapultWars copies of this framework, since neither
  of those samples is touch/gesture-driven. Don't add gesture routing to those samples'
  copies unless they actually gain a touch-driven feature; keep each sample's copy of the
  framework only as complex as that sample needs.

### CatapultWars specifics
- `Catapult` ↔ `Player` is a reference cycle: `Catapult` holds non-owning `Player*`
  back-pointers and declares the Player-dependent methods (`CheckHit`, `getEnemyScore`),
  defined **out-of-line in `Player.hpp`** after `Player` is a complete type. `Player`
  owns its `Catapult` via `std::shared_ptr`.
- `CatapultState` is a bit-flag `enum class` with `constexpr operator|`/`operator&`.
- Animations advance one frame per `Update`; the game runs at a **fixed 30 fps
  timestep** (`setTargetElapsedTimeProperty(FromSeconds(1.0/30.0))`), or they play twice
  as fast.

### Touch/gesture input — three established fallback patterns, pick by interaction shape
CNA has a real `Microsoft::Xna::Framework::Input::Touch::TouchPanel` (gestures detected by
`CNA::Internal::Input::GestureDetector`, real SDL3 finger events wired in
`SdlInputBridge.cpp`) and a real `Microsoft::Devices::Sensors::Accelerometer` (SDL3
`SDL_Sensor`-backed) — both proven end-to-end by Yacht/GesturesSample/TouchThumbsticks/
SnowShovel. Since this desktop has no touchscreen and CNA does not synthesize touch from
mouse input, every touch/gesture-driven sample adds a mouse fallback. Three shapes have
emerged so far — **use whichever matches a new sample's mechanic, don't invent a fourth**:
1. **Single contact point, drag-shaped** (GesturesSample): a per-sample keyboard/mouse
   fallback struct alongside the real touch handling.
2. **Two simultaneous contact points** (TouchThumbsticks): keyboard (one stick) + mouse
   (the other), since one mouse pointer can't emulate two fingers.
3. **Every interaction is a discrete tap** (DynamicMenu): synthesize one `GestureType::Tap`
   per mouse-click rising edge at the Game level, feed it into the same gesture list real
   touches use — every tappable control gets it for free, zero library changes.
4. **Continuous drag/flip/scroll, not just taps** (UISample): centralize *all* synthesis —
   raw `TouchLocation(Pressed)` + `Tap` on press, `HorizontalDrag`/`VerticalDrag` per frame
   while held, `DragComplete` on release — in the shared `InputState::Update()`/
   `UpdateMouseFallback()`, so every consumer of `input.Gestures`/`input.TouchState`
   (MenuScreen taps, ScrollTracker, PageFlipTracker) gets mouse support with zero
   per-control changes. Prefer this over pattern 1 whenever a sample has more than one
   gesture-driven control type, or any drag/flick (not just tap).
- SnowShovel's `Accelerometer.hpp` is the minimal accelerometer-wrapper version of pattern
  1 (no shake detection, just `Initialize()` + `GetTilt() -> std::optional<Vector3>`) —
  start there, not Yacht's fuller wrapper, if a future sample only needs raw tilt.
- **Gotcha:** a sample must call `TouchPanel::setDisplayWidthProperty`/
  `setDisplayHeightProperty` itself in `Initialize()` — there is no auto-wiring from the
  back-buffer size — or gesture positions/thresholds come out wrong (zero, in UISample's
  case — see section 5).

### DynamicMenu control library (Controls/*.hpp, Transitions/Transition.hpp)
- **Invariant — do not change `Control::activeTransitions_` back to
  `std::vector<Transition>` (by value).** The original C# `Transition` is a reference
  type, and `Control.Update` copies its transitions list before iterating specifically so
  a `TransitionComplete` callback can call `ApplyTransition` (adding a new transition)
  *during* that iteration without corrupting it (the Get-Big button's completion handler
  does exactly this — it re-triggers a second transition from its own completion
  callback). The port uses `std::vector<std::shared_ptr<Transition>>` to get the same
  effect: copying the list of shared pointers is safe against the vector reallocating
  mid-iteration, while the pointed-to `Transition` objects stay shared. This was a real
  bug caught by static review, not by live testing — see `samples/DynamicMenu/missing.md`.
- `IControl`/`ITextControl` were merged into `Control`/`TextControl` respectively — nothing
  in this sample implements either interface independently, and C++ has no properties
  mechanism for an interface to declare the way C#'s auto-properties do. Don't reintroduce
  the split without a concrete second implementer that needs it.
- Page 2/3's control trees were originally XML content-pipeline assets; they're hand-built
  in C++ (`BuildMenuPage2Content()`/`BuildMenuPage3Content()` in `DynamicMenuGame.hpp`)
  since CNA has no content-pipeline XML deserializer. If a future sample also has
  XML-authored content-pipeline assets, this same "hand-translate once" approach is the
  established precedent — don't write a general XML deserializer for one sample's sake.

### UISample control library (Controls/*.hpp) and screen framework
- This is a **second, independent** UI control library, unrelated to DynamicMenu's — no
  code sharing (matching "no shared sample library"), and its `Control`/`TextControl` etc.
  happen to look structurally similar but are separately ported from a different original.
- **Invariant — keep calling `TouchPanel::setDisplayWidthProperty`/`HeightProperty` in
  `Game::Initialize()`.** `PageFlipTracker`/`ScrollTracker` both read
  `TouchPanel::DisplayWidth`/`DisplayHeight` directly; forgetting this call (as the first
  draft of this sample did) makes every drag-to-flip/scroll threshold compute against 0 —
  a real bug caught by interactive testing, fixed by using the known back-buffer constants
  directly in `Initialize()` rather than querying the viewport that early (same root cause
  as the SnowShovel `Viewport` gotcha above, different property).
- Tombstoning (`SerializeState`/`DeserializeState`/`IsSerializable`) and the debug
  `TraceScreens()` helper are dropped from `ScreenManager`, matching the same simplification
  already applied when GameStateManagement itself was ported.
- Uses pattern 4 from the touch/gesture note above (`InputState::UpdateMouseFallback()`)
  for all its mouse fallback.

---

## 7. Useful commands

```bash
# Configure (first time)
cmake --preset debug            # or: cmake --preset debug-easygl

# Build everything / one sample (rebuilds CNA and sharp-runtime as needed)
cmake --build cmake-build-debug
cmake --build cmake-build-debug --target UISample_cna_samples

# Run a sample (run from its own binary dir so Content/ is found)
cd cmake-build-debug/samples/UISample && ./UISample_cna_samples

# Reproduce the main open bug (CameraShake white stripe)
./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples

# Generate a SpriteFont atlas (ALWAYS pass --content-root; --chars for non-default charsets)
python3 tools/make_font.py /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf 24 \
    samples/X/Content/Fonts/MenuFont --content-root samples/X/Content

# Generate the F1 help overlay PNG from the sample's .htm
python3 tools/gen_help_png.py samples/X/X.htm samples/X/Content/help.png

# Headless screenshot of an SDL3 app (native-Wayland windows are invisible to X11 tools)
SDL_VIDEODRIVER=x11 ./cmake-build-debug/samples/X/X_cna_samples &
WID=$(xwininfo -root -tree | grep -i X_cna_samples | grep -oE '0x[0-9a-f]+' | head -1)
import -window "$WID" /tmp/shot.png

# Interactive verification with xdotool (installed; re-verify focus immediately before
# EVERY input burst, not just once -- see the gotchas in section 5)
xdotool windowactivate --sync "$WID"
xdotool getactivewindow   # must print $WID (as a decimal) -- if not, stop, don't click
xdotool mousemove "$X" "$Y"
xdotool click 1                                      # one xdotool action per shell call
xdotool mousedown 1; sleep 0.5; xdotool mouseup 1     # explicit down/sleep/up for drags
xdotool key F1
```

There is no lint/format command configured in this repo, and no automated test command
(see section 2).

---

## 8. Next smallest tasks

1. **(done this session)** ~~Interactively re-verify DynamicMenu (#077) and UISample's
   HighScoreScreen (#082).~~ DynamicMenu's Page 2/3, progress bar `Advance`, and `O`
   orientation toggle are now screenshot-confirmed working (see section 3/4C/5). UISample's
   HighScoreScreen scroll could not be drag-tested — blocked by an `xdotool`
   mouse-button-held position freeze specific to this desktop, not a code bug (see section
   4C). If a definitive answer on the scroll is ever needed, retest with real hardware
   input rather than `xdotool`.

2. **(done this session)** ~~Port the next portable Phase 7 sample.~~ PerformanceMeasuring
   #081 is done (section 3). NGSMSample #075 and SplitScreen #076, the two remaining
   Phase 7 samples, were both investigated and marked Deferred in PLAN.md with precise
   reasons (section 1, section 4D, `samples/SplitScreen/missing.md`) — Phase 7 has no
   more candidates until CNA gains networking (#075) or per-mesh `ModelBone` support
   (#076, TankOnHeightmap #074 same gap — the user is handling that fix directly in
   `cna`, not this repo).

3. **(done this session)** ~~Port a Phase 6 full game.~~ SoccerPitch #073 and
   HoneycombRush #063 are both done (section 3) — surveyed all 11 then-open Phase 6
   samples for blockers first (section 1). Good remaining candidates with no found
   blockers: **NinjAcademy #065**, **CardsStarterKit #069**.
   - Also worth a follow-up: independently re-confirm PerformanceMeasuring's `Tab`-to-close
     and `Up`/`Down` sphere-count controls, cut short this session by real user keystrokes
     crossing into the test window (see section 3/5's newest gotcha).
   - And once the `cna` repo gains the per-mesh-bone reader fix described in
     `samples/SplitScreen/missing.md`: come back and actually port SplitScreen (copy
     `tank.model.json`+bins straight from `samples/CameraShake/Content/`, no new asset
     conversion needed) — it's otherwise a ~450-line, 3-file sample, one of the smallest
     remaining. TankOnHeightmap #074 would then also be unblocked.

4. **Investigate CameraShake's near-plane clipping bug in the EasyGL backend.**
   - Goal: clip w<0 vertices the way DirectX does so the ground/tank actually render.
   - Files: likely `cna/.../EasyGL/EasyGLGraphicsBackend.cpp` (clipping/projection path)
     — exact location not yet confirmed, this needs investigation first.
   - Verify: the white stripe disappears when running
     `./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples`.

5. **Fix the Vulkan multiple-SpriteBatch-per-frame bug.**
   - Goal: a second `Begin/End` in the same frame must not discard the first.
   - Files: `cna/.../Vulkan/VulkanGraphicsBackend.cpp`.
   - Verify: run GameStateManagement or CatapultWars on the Vulkan backend; confirm all
     layers draw (currently only the last `Begin/End` block's sprites appear).

6. **Add `CMakeLists.txt` + `CONTENT_DIR` to a deferred sample once its blocker lifts.**
   - Goal: when a Model/Effect pipeline exists (task 4/5 area, not yet started), enable
     one of BloomSample / ColorReplacement / ReachGraphicsDemo / Spacewar with
     `CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Content`.
   - Files: `samples/<Name>/CMakeLists.txt`, root `CMakeLists.txt`.
   - Verify: the target builds and the binary does not abort on `help.png` load at
     startup.

7. **Verify real hardware accelerometer shake/tilt on a device that has one.**
   - Goal: confirm Yacht's and SnowShovel's real-sensor code path
     (`Accelerometer::getIsSupportedProperty()` true branch), only exercised via the
     keyboard-arrow/gamepad/touch fallback so far.
   - Files: `samples/Yacht/src/Accelerometer.hpp`, `samples/SnowShovel/src/Accelerometer.hpp`.
   - Verify: run on a machine/device with a physical accelerometer; shake/tilt it; confirm
     dice roll (Yacht) / shovel movement (SnowShovel) triggers.

---

## 9. Do not do yet

- **Do not start Phase 3 samples** without first establishing an HLSL→GLSL
  `.shader.json` workflow — no HLSL runtime compiler exists in CNA.
- **Do not start Phase 4 samples** without a proven model-conversion path to
  `.model.json`.
- **Do not add a shared `samples/common/` library** — each sample must stay
  self-contained, even where two samples' UI-control libraries end up structurally
  similar (DynamicMenu vs. UISample) — that similarity is coincidental (different
  originals), not a reason to unify them.
- **Do not change the CameraShake ground scale / camera** to hide the white stripe —
  fix the underlying CNA clipping bug instead.
- **Do not work around CNA bugs inside a sample** (no per-control scaling, sample-side
  letterboxing, manual UTF-8 pre-encoding tricks, etc.). If CNA diverges from real XNA
  4.0 behavior, check FNA (`/rv/data/library/github.com/FNA-XNA/FNA`) as the reference,
  **verify by reading the actual CNA header first** (a wrong "CNA doesn't support X" claim
  was made and caught this session — see section 5's last row), and fix it in the `cna`
  repo if it's real — but always confirm the fix and its scope with the user before
  touching a shared framework repo, since other work may be happening there concurrently.
- **Do not switch `ScreenManager` screen ownership** from `shared_ptr` to raw pointers.
- **Do not re-generate existing font atlases** unless there is a confirmed rendering
  bug — regenerating is cheap but pointless churn otherwise.
- **No broad refactors or unrelated cleanup** while the CameraShake clipping bug and the
  Vulkan multi-SpriteBatch bug are open.
- **Do not assume `git status` is clean in `cna`/`sharp-runtime` before editing them** —
  they are independently active sibling repos; check `git log`/`git status` there first.
- **Do not hammer `xdotool` input without re-checking focus first** (section 5) — a
  misdirected click can land on the user's own foreground application, and a malformed
  multi-line xdotool shell block can get chained into one runaway background command.

---

## 10. Resume prompt

```
Read NEXT.md first to understand the current project state.
Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not start any task not listed in section 8.
Make one small, verified improvement, then run the relevant build/test command
(e.g. cmake --build cmake-build-debug --target <Name>_cna_samples) to confirm it.
After finishing, update NEXT.md (status, recent changes, next tasks) to match reality.
```
