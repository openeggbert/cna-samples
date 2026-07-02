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
(GameStateManagement #072, CatapultWars #067, Yacht #071 done). Phase 7 (Advanced/UI/Misc)
is underway (GesturesSample #079, TouchThumbsticks #080, LocalizationSample #078 done).
Phases 3 (3D shaders) and 4 (models/animation) are untouched, blocked on missing asset
pipelines (see section 4B).

**Key architectural decisions:**
- One executable per sample; no shared sample library. Each sample directory is
  self-contained.
- All sample code is header-only except each sample's `src/Program.cpp`.
- Assets: `Load<Texture2D>` → PNG; `Load<SpriteFont>` → `.font.json` + PNG atlas (via
  `tools/make_font.py`); `Load<SoundEffect>` → WAV; `Load<Song>` → OGG/WAV/MP3/FLAC. XNA
  `.xnb` is never supported and never will be.
- `CONTENT_DIR` in each sample's `cna_add_sample()` call is **mandatory** for any sample
  that loads assets (including the F1 `help.png` overlay) — otherwise the binary aborts
  at startup.
- Default graphics backend is **EasyGL** (OpenGL ES); a Vulkan backend also exists but has
  a known bug (section 5).
- CNA is consumed via `add_subdirectory(../cna)`, so building any sample also rebuilds
  CNA (and transitively sharp-runtime) if their sources changed.
- `cna` and `sharp-runtime` are separate sibling git repositories, not submodules of this
  one. Framework-level bugs found while porting a sample get fixed in those repos
  directly (see section 3) — never worked around inside a sample.

---

## 2. Current status

### Build
All 34 enabled samples compiled and linked cleanly with the default **EasyGL** backend as
of the last full build in this session (`cmake --build cmake-build-debug`, 0 errors). The
active configured build tree is `cmake-build-debug`. This has not been re-verified since
the last commit in section 3 below — treat as "known good as of that commit," not as a
live guarantee.

### Tests
No automated test suite exists in this repo. The samples themselves are the manual/visual
integration tests: verification is done by building a target, running it (optionally
under `SDL_VIDEODRIVER=x11` for headless screenshotting), and inspecting a screenshot or
driving it interactively with `xdotool`.

### CLI / tools available
- `tools/make_font.py` — generates a CNA `.font.json` + PNG glyph atlas from a TTF/TTC
  file and an explicit or default character set.
- `tools/gen_help_png.py` — extracts a sample's "Sample Controls" table from its `.htm`
  doc and renders the F1 help-overlay PNG.
- No linter/formatter is configured in this repo.

### Recently implemented features
- LocalizationSample (#078): 5-language (en/da/fr/ja/ko) string + flag switching, SPACE
  cycles language.
- TouchThumbsticks (#080): twin-stick space shooter with real dual-touch virtual
  thumbsticks.
- GesturesSample (#079): real `TouchPanel` gesture recognition (Hold/Tap/Drag/Flick/Pinch).
- Two CNA framework fixes (asset path resolution; UTF-8 text decoding) — see section 3.

### Known working examples / demos
34 samples run end-to-end; see the full table in the previous version of this file's
git history, or run `cmake --build cmake-build-debug` and look under `samples/*/` for the
current set. Representative, recently-verified ones:
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

---

## 3. Recent changes

Most recent first, matching `git log`:

- **`d90ec24`** — Added `samples/LocalizationSample/` (#078): English/Danish/French/
  Japanese/Korean welcome text + flag, switched via a `LoadLocalizedAsset<T>` fallback
  (try full culture name, then language-only, then default). sharp-runtime's
  `CultureInfo.CurrentCulture` is a stub (always invariant, no real OS locale detection),
  so SPACE manually cycles the 5 languages instead. New files:
  `samples/LocalizationSample/{CMakeLists.txt,missing.md,LocalizationSample.htm,
  src/{Program.cpp,LocalizationGame.hpp,Strings.hpp},Content/...}`.
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
  parallel mouse fallback (drag/click/hold-timer/scroll-wheel) for desktop testing. One
  bug caught during testing and fixed in the sample itself: holding the mouse button
  while scrolling could cross the hold-timer threshold and delete the sprite mid-scale;
  scroll input now also suppresses the hold timer, same as dragging does.
- Earlier history (Yacht #071 real touch/accelerometer, CatapultWars #067,
  GameStateManagement #072, and the Phase 1/2/5 foundational samples) — see `git log`
  and `PLAN.md` for the full list; not repeated here to keep this section current.

---

## 4. Current blocker / main problem

There is **no single hard blocker** stopping all progress — new portable 2D/Phase 7
samples can still be ported freely. The two most significant *open* problems are:

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
| needs verification | **Real hardware accelerometer** — Yacht's shake-to-roll is only verified via the keyboard-arrow fallback on this desktop (no physical accelerometer available). The real-sensor code path (`Accelerometer::getIsSupportedProperty()` true branch) is implemented but untested end-to-end on real hardware. |
| gotcha | **`xdotool` input can silently no-op if the target window lost X focus** (e.g. after an intervening screenshot/tool call) — clicks/keys are sent but never reach the app, with no error message. Always run `xdotool windowactivate --sync $WID && xdotool windowfocus --sync $WID` immediately before each simulated input burst. A plain `xdotool click`/`key` can also complete faster than one game-loop frame (~33ms at the sample's 30fps) and be missed entirely by a naive down/up edge check — prefer explicit `keydown`/`sleep 0.15`/`keyup` over `click`/`key` when the app must observe a discrete press. |
| fixed (in `cna`, not this repo) | `ContentManager::ResolveAssetPath` misresolved asset names with a non-extension dot (e.g. `"Flag.en-US"`). See section 3, commit `80757b1` in the `cna` repo. |
| fixed (in `cna`, not this repo) | `SpriteFont`/`SpriteBatch::DrawString` had no UTF-8 decoding, so any non-ASCII text rendered as `?`. See section 3, commit `41a4766` in the `cna` repo. |

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
  use ordinary `getFoo()`/`setFoo()` without the "Property" suffix — that suffix is
  reserved for real CNA/XNA framework types generated via the `DEF_PROP` macro.)
- Every `Game` **and** `DrawableGameComponent` subclass must implement
  `const std::string& GetTypeName() const override`.
- `Content.Load<T>()` returns **T by value** — store results in `std::optional<T>` (or a
  plain member with a no-arg default constructor, since `Texture2D` etc. share GPU
  backends via an internal `shared_ptr` and are cheap to copy). Anything that keeps a raw
  pointer *into* such a value (e.g. a sprite-sheet `Animation` referencing a shared
  texture) must ensure that value lives at a **stable address**
  (`std::optional`/`std::map` node, never a reallocating `std::vector`).
- Virtual overrides on `Game`: `void Update(GameTime&)` (non-const) and
  `void Draw(const GameTime&)` (const). `DrawableGameComponent::Initialize()` calls
  `LoadContent()` once.
- No `operator+=` for `Vector2`/`TimeSpan` — write `v = v + expr`. `Color` has no
  `operator*` — multiply channels manually.
- `Rectangle` exposes public `X/Y/Width/Height` and `Contains(x, y)` (no
  `getXProperty`). `Point`/`Vector2` expose public `X/Y` directly.
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

### ScreenManager framework (shared shape of GameStateManagement & CatapultWars)
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

### CatapultWars specifics
- `Catapult` ↔ `Player` is a reference cycle: `Catapult` holds non-owning `Player*`
  back-pointers and declares the Player-dependent methods (`CheckHit`, `getEnemyScore`),
  defined **out-of-line in `Player.hpp`** after `Player` is a complete type. `Player`
  owns its `Catapult` via `std::shared_ptr`.
- `CatapultState` is a bit-flag `enum class` with `constexpr operator|`/`operator&`.
- Animations advance one frame per `Update`; the game runs at a **fixed 30 fps
  timestep** (`setTargetElapsedTimeProperty(FromSeconds(1.0/30.0))`), or they play twice
  as fast.

### Touch/gesture/thumbstick input (Yacht, GesturesSample, TouchThumbsticks)
- CNA has a real `Microsoft::Xna::Framework::Input::Touch::TouchPanel` (gestures
  detected by `CNA::Internal::Input::GestureDetector`, real SDL3 finger events wired in
  `SdlInputBridge.cpp`) and a real `Microsoft::Devices::Sensors::Accelerometer`
  (SDL3 `SDL_Sensor`-backed). Both are proven end-to-end by these three samples.
- **Gotcha:** a sample must call `TouchPanel::setDisplayWidthProperty` /
  `setDisplayHeightProperty` itself in `Initialize()` — there is no auto-wiring from the
  backbuffer size — or gesture positions come out at (0, 0).
- Since this desktop has no touchscreen and CNA does not synthesize touch from mouse
  input, every touch-driven sample in this repo adds a parallel keyboard/mouse fallback
  for desktop testing. Where the original mechanic needs only one contact point
  (GesturesSample), a mouse fallback suffices; where it fundamentally needs two
  simultaneous contacts (TouchThumbsticks), a keyboard+mouse combination is used
  instead. This is a deliberate, per-sample, documented addition (see each sample's
  `missing.md`) — never a workaround for a CNA limitation.

---

## 7. Useful commands

```bash
# Configure (first time)
cmake --preset debug            # or: cmake --preset debug-easygl

# Build everything / one sample (rebuilds CNA and sharp-runtime as needed)
cmake --build cmake-build-debug
cmake --build cmake-build-debug --target LocalizationSample_cna_samples

# Run a sample (run from its own binary dir so Content/ is found)
cd cmake-build-debug/samples/LocalizationSample && ./LocalizationSample_cna_samples

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

# Interactive verification with xdotool (installed; refocus before every input burst,
# see the gotcha in section 5)
xdotool windowactivate --sync "$WID"
xdotool windowfocus --sync "$WID"
xdotool mousemove --window "$WID" 200 200
xdotool mousedown 1; sleep 0.5; xdotool mouseup 1   # explicit down/sleep/up, not `click`
xdotool key F1
```

There is no lint/format command configured in this repo, and no automated test command
(see section 2).

---

## 8. Next smallest tasks

1. **Port the next portable Phase 7 sample.**
   - Goal: extend sample coverage. Real `TouchPanel`/`Accelerometer` and non-ASCII
     `SpriteFont` text are all confirmed working end-to-end now, so no remaining Phase 7
     candidate should be deprioritized for those reasons.
   - Candidates, smallest first by original C# line count: SnowShovel #083,
     SplitScreen #076, DynamicMenu #077, PerformanceMeasuring #081, UISample #082,
     NGSMSample #075. (Phase 6 full games also have open items — see `PLAN.md` — if a
     bigger port is preferred instead.)
   - Files: new `samples/<Name>/` directory; one line added to root `CMakeLists.txt`.
   - Verify: `cmake --build cmake-build-debug --target <Name>_cna_samples`, then run and
     screenshot it (see section 7).

2. **Investigate CameraShake's near-plane clipping bug in the EasyGL backend.**
   - Goal: clip w<0 vertices the way DirectX does so the ground/tank actually render.
   - Files: likely `cna/.../EasyGL/EasyGLGraphicsBackend.cpp` (clipping/projection path)
     — exact location not yet confirmed, this needs investigation first.
   - Verify: the white stripe disappears when running
     `./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples`.

3. **Fix the Vulkan multiple-SpriteBatch-per-frame bug.**
   - Goal: a second `Begin/End` in the same frame must not discard the first.
   - Files: `cna/.../Vulkan/VulkanGraphicsBackend.cpp`.
   - Verify: run GameStateManagement or CatapultWars on the Vulkan backend; confirm all
     layers draw (currently only the last `Begin/End` block's sprites appear).

4. **Add `CMakeLists.txt` + `CONTENT_DIR` to a deferred sample once its blocker lifts.**
   - Goal: when a Model/Effect pipeline exists (task 2/3 area, not yet started), enable
     one of BloomSample / ColorReplacement / ReachGraphicsDemo / Spacewar with
     `CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Content`.
   - Files: `samples/<Name>/CMakeLists.txt`, root `CMakeLists.txt`.
   - Verify: the target builds and the binary does not abort on `help.png` load at
     startup.

5. **Verify real hardware accelerometer shake on a device that has one.**
   - Goal: confirm Yacht's real-sensor code path
     (`Accelerometer::getIsSupportedProperty()` true branch), only exercised via the
     keyboard-arrow fallback so far.
   - Files: `samples/Yacht/src/Accelerometer.hpp`.
   - Verify: run on a machine/device with a physical accelerometer; shake it; confirm
     dice roll triggers.

---

## 9. Do not do yet

- **Do not start Phase 3 samples** without first establishing an HLSL→GLSL
  `.shader.json` workflow — no HLSL runtime compiler exists in CNA.
- **Do not start Phase 4 samples** without a proven model-conversion path to
  `.model.json`.
- **Do not add a shared `samples/common/` library** — each sample must stay
  self-contained.
- **Do not change the CameraShake ground scale / camera** to hide the white stripe —
  fix the underlying CNA clipping bug instead.
- **Do not work around CNA bugs inside a sample** (no per-control scaling, sample-side
  letterboxing, manual UTF-8 pre-encoding tricks, etc.). If CNA diverges from real XNA
  4.0 behavior, check FNA (`/rv/data/library/github.com/FNA-XNA/FNA`) as the reference
  and fix it in the `cna` repo, as was done twice this session (section 3) — but always
  confirm the fix and its scope before touching a shared framework repo, since other
  work may be happening there concurrently.
- **Do not switch `ScreenManager` screen ownership** from `shared_ptr` to raw pointers.
- **Do not re-generate existing font atlases** unless there is a confirmed rendering
  bug — regenerating is cheap but pointless churn otherwise.
- **No broad refactors or unrelated cleanup** while the CameraShake clipping bug and the
  Vulkan multi-SpriteBatch bug are open.
- **Do not assume `git status` is clean in `cna`/`sharp-runtime` before editing them** —
  they are independently active sibling repos; check `git log`/`git status` there first.

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
