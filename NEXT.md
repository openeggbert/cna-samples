# NEXT.md

## 1. Project summary

**What this is:** C++ ports of the official Microsoft XNA Game Studio 4.0 sample
collection, running on **CNA** (`../cna`) — a C++ reimplementation of the XNA 4.0 API
built on SDL3, itself built on **sharp-runtime** (`../sharp-runtime`), a C++ port of
relevant .NET BCL types.

**Main goal:** Port the applicable XNA 4.0 desktop samples (PLAN.md catalogs ~130+
XNA Game Studio samples total) to CNA C++, preserving the original class hierarchy and
naming (`Microsoft::Xna::Framework::*`). The ported samples double as integration tests
for CNA and as a migration reference for anyone porting XNA/MonoGame code to CNA.

**Current phase:** The great majority of readily-portable samples are done — 45 sample
targets are wired into the root `CMakeLists.txt` and build. Every other XNA 4.0 sample
worth tracking now has a `samples/<Name>/` placeholder directory (`<Name>.htm` +
`missing.md`, no source yet) documenting exactly which CNA gap blocks it — 41 total,
see `PLAN.md`'s Sample Count Summary. The 67 samples that will never be ported (not
real XNA 4.0 games, WinForms/content-pipeline tools, redundant training-kit
duplicates, or tied to a platform CNA won't ever target) are catalogued in
`ignored.md` instead. Remaining un-ported samples are blocked on real CNA engine work
(an HLSL→GLSL shader pipeline, skeletal animation playback, a `NetworkSession`-alike,
microphone capture, or — for exactly one sample, CustomModelEffect — a content-
pipeline processor CNA has no equivalent of at all) or, for AccelerometerSample
(#084)/TiltPerspective (#107), a product/scope decision rather than a technical
gap — see section 4.

**Key architectural decisions:**
- One executable per sample; no shared sample library. Each `samples/<Name>/` directory
  is fully self-contained (`src/` header-only except one `Program.cpp`, `Content/`,
  `missing.md`, a verbatim copy of the original `.htm` doc) — even where two samples'
  code ends up structurally similar.
- Assets: `Load<Texture2D>` → PNG; `Load<SoundEffect>`/`Load<Song>` → WAV/OGG;
  `Load<SpriteFont>` → `.font.json` + PNG atlas (`tools/make_font.py`); `Load<Model>` →
  `.model.json` (static/rigid single-bone models only — see section 5). XNA `.xnb` is
  never supported. Small numbers of custom XML content files are hand-translated to C++
  construction code; the one exception is RolePlayingGame's 281 data files, which needed
  a real runtime XML loader instead (see section 6).
- CNA is consumed via `add_subdirectory(../cna)`, so building any sample also rebuilds
  CNA (and transitively sharp-runtime) if their sources changed. `cna` and
  `sharp-runtime` are independently active sibling repos, not submodules — check their
  own `git log`/`git status` before assuming a clean state, and confirm scope with the
  user before editing either.
- Default graphics backend is **EasyGL** (OpenGL ES); a Vulkan backend also exists but
  has a known bug (section 5).
- Framework-level gaps found while porting a sample get fixed in `cna`/`sharp-runtime`
  directly (after confirming scope with the user) — never worked around inside a
  sample.

---

## 2. Current status

### Build
46 of the samples wired into `CMakeLists.txt` compile and link cleanly against the
default **EasyGL** backend, as of commit `92eef38` on `develop` (pushed). The five
samples added in the most recent session — NinjAcademy (#065), CardsStarterKit (#069),
RolePlayingGame (#070), Particles2DPipeline (#044), Orientation (#102) — were each
independently confirmed to build with 0 errors via a live `cmake --build` run (not
assumed from memory). A full "all targets" build (`cmake --build cmake-build-debug
-j$(nproc)`) now succeeds end to end (0 errors, confirmed live this session) —
**InputReporter**'s build failure (stale direct `GamePadCapabilities` field access,
e.g. `cap.HasLeftStickButton`, after upstream `cna` commit `2520109` made the fields
private behind `getXxxProperty()`) was fixed by switching every access in
`samples/InputReporter/src/InputReporterGame.hpp`'s `DrawData()` to the
`getXxxProperty()` accessors. The active configured build tree is `cmake-build-debug`;
configuring may need `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` on newer CMake (a vendored
ENet `CMakeLists.txt` inside `cna`'s networking dependency requires it).

### Tests
No automated test suite exists in this repo. Each sample is its own manual/visual
verification unit: build it, run it, and (when input can be verified — see section 5)
interact with it.

### Tools / libraries currently available
- `tools/make_font.py <font.ttf> <size_px> <Content/FontName>` — generates a
  `.font.json` + atlas PNG SpriteFont asset.
- `tools/gen_help_png.py <Sample.htm> <Content/help.png>` — extracts a sample's own
  `.htm` "Sample Controls" table into the mandatory F1 help-overlay PNG.
- `tools/obj2model.py`, `tools/fbx_ascii2model.py` — convert static, single-bone
  `.obj`/`.fbx` models to CNA's `.model.json` format.

### Recently implemented / working
See section 3 for the most recent session's additions. Notable working subsystems
proven across multiple samples: real touch/gesture input (`TouchPanel`), real
`Microsoft::Devices::Sensors::Accelerometer` (SDL_Sensor-backed, works on
Android/iOS/desktop when physical hardware is present, with an established
keyboard/gamepad fallback pattern for when it isn't), `SpriteFont`/`DrawString`, audio
(`SoundEffect`/`Song`/`SoundEffectInstance` via SDL3_mixer), `RenderTarget2D`, static
model loading and rendering, and `GraphicsDeviceManager.SupportedOrientations`/
`GameWindow.CurrentOrientation` (portrait/landscape switching).

### Known NOT working
- Interactive input verification via `xdotool` on this development desktop is
  unreliable (see section 5) — this blocks live-testing new samples' keyboard/mouse
  interactions, not the samples themselves.
- Custom HLSL shader effects (Phase 3, most samples) and skeletal animation playback
  (most of Phase 4) — real, unimplemented CNA engine features, not asset-conversion
  gaps. See DEFERRED.md items 11 and 13.

---

## 3. Recent changes (most recent session)

### This session
- Fixed `InputReporter`'s full-repo build failure (stale direct `GamePadCapabilities`
  field access after upstream `cna` made the fields private) — see section 2.
- **Created `ignored.md`**: a new top-level file listing all 67 XNA sample
  directories (of 153 total) that will **never** get a `samples/` directory —
  archived XNA 2.0/3.0/3.1 samples, WinForms/content-pipeline-only tools, redundant
  `*TrainingKit` duplicates of already-ported games, Xbox LIVE Avatar/Bing Maps/
  phone-OS-only samples, and other non-code/non-4.0/non-runnable-Game directories —
  each with a one-line reason. This is now the authoritative "why isn't this here"
  reference, so `PLAN.md`'s old per-category tables (archives, avatar packs, phone/
  Mango variants, VB dups, Silverlight/WP7, image-only, third-party, unversioned
  starters, misc) were collapsed into a single pointer at `PLAN.md`'s bottom instead
  of duplicating all 42 of those rows in both files.
- **Added 36 new placeholder sample directories** (`<Name>.htm` verbatim + a
  `missing.md` documenting the exact CNA blocker, no `src/`/`CMakeLists.txt`) for
  every remaining XNA 4.0 sample judged to "make sense once CNA can do more" —
  dispatched as 7 parallel background agents, each given the precise blocker/
  `DEFERRED.md` item to cite (pre-verified via direct C# source grep before
  dispatch, not left to the agents to discover from scratch): all 17 remaining
  Phase 3 samples, all 9 Phase 4 samples, the 4 remaining Phase 6 samples
  (MarbleMaze/NetRumble/ShipGame/TankOnHeightmap), and 6 of the 27 "Deferred —
  Phone Hardware" appendix samples (AccelerometerSample, TiltPerspective,
  MicrophoneEcho, ClientServerSample, NetworkPrediction, PeerToPeer — the appendix's
  other 21 went to `ignored.md` instead). Two samples (MarbleMaze, CPUSkinning) turned
  out to have no `.htm` at all in their source tree (only a `.doc` tutorial) — the
  agents correctly documented that fact in `missing.md` rather than fabricating one.
  Root `CMakeLists.txt` got matching commented-out `add_subdirectory()` lines for all
  36. `PLAN.md`'s per-sample status column and summary tables were updated to match
  (45 Done / 41 Placeholder / 67 Ignored = 153 total).
- Added 3 new `DEFERRED.md` gap items found/needed during the above: **#16**
  (Microphone capture — blocks MicrophoneEcho), **#17** (Multiplayer networking /
  `NetworkSession`-alike — blocks NetRumble + the 3 newly-added networking samples),
  **#18** (content-pipeline processor extensibility — a genuinely new, deeper class of
  gap than shader/model conversion, found while documenting CustomModelEffect, which
  bakes a reflection cubemap via a custom build-time MSBuild processor chain CNA has
  no equivalent of at all).
- **Not yet done this session** (next up): audit all 45 already-ported samples'
  `missing.md` files for completeness — the user's ask was that every sample,
  ported or not, should have its CNA gaps fully documented, and the 45 existing ones
  were written at various points over many sessions without this session's more
  rigorous direct-source-audit standard.

### Previous session
- Added `samples/NinjAcademy/` (#065), `samples/CardsStarterKit/` (#069),
  `samples/RolePlayingGame/` (#070) — full ScreenManager-based games. RolePlayingGame
  needed a hand-rolled runtime XML loader (`src/Xml/XmlNode.hpp` +
  `src/Data/ContentLoader.hpp`) for its 281 data files, a deliberate, user-approved
  deviation from this repo's usual "hand-translate a handful of XML files into C++"
  convention, which doesn't scale past a few files.
- Audited every remaining un-ported XNA sample in Phase 3/4/6 (~30 samples) against
  actual C# source instead of trusting DEFERRED.md's own prior summary claims, and
  found two of them were stale/overbroad:
  - DEFERRED.md item 5 (`VertexPositionNormal` + lit shader) actually blocks 9 more
    samples than previously listed (LensFlareSample, Graphics3DSample, PickingSample,
    TrianglePickingSample, HeightmapCollisionSample, CustomModelClassSample,
    InverseKinematics, ChaseCamera, MarbleMaze) — none of them has a custom `.fx`
    shader, only per-vertex-lit `BasicEffect`.
  - New DEFERRED.md item 14: RimLighting only needs a `Content.Load<TextureCube>`
    reader added to `ContentManager.cpp` (effort S), not the full shader pipeline.
  - New DEFERRED.md item 15: corrected the "Deferred — Phone Hardware" reasoning for
    AccelerometerSample/TiltPerspective/Orientation — CNA's `Accelerometer` is a real,
    working, cross-platform (Android/iOS/Desktop) `SDL_Sensor` implementation, not an
    Android-only stub.
- Added `samples/Particles2DPipeline/` (#044) and `samples/Orientation/` (#102) — two
  samples the audit found were genuinely unblocked despite living in "hard" phases.
- Found and fixed a real bug in Orientation's `O` orientation-cycle during live
  interactive testing (triggered by a user report of "O does nothing"): the cycle
  visited all 3 `DisplayOrientation` values, but `LandscapeLeft`/`LandscapeRight`
  render pixel-identical (the `directions` texture has no rotation transform, matching
  the original) — half of all `O` presses looked like no-ops. Fixed to a plain
  Landscape/Portrait 2-state toggle and confirmed live afterward.
- PLAN.md: rows 044, 065, 069, 070, 102 flipped to ✅ Done; row 102 moved out of the
  "Deferred — Phone Hardware" appendix into the Phase 7 table; row 031 (BloomSample)
  had a stale "+ RenderTarget2D" blocker clause trimmed (RenderTarget2D is resolved).
- Fixed `InputReporter`'s full-repo build failure (section 4c/8 item 1): its
  `DrawData()` still read `GamePadCapabilities` as raw public fields
  (`cap.HasLeftStickButton`, etc.), which matched `cna`'s original struct layout but
  broke when upstream commit `2520109` ("rework GamePadCapabilities to
  getXProperty()/NOXNA setters") made every field private behind a `getXxxProperty()`
  accessor — confirmed via FNA (`GamePadCapabilities.cs`) that XNA 4.0 itself exposes
  these as `{ get; internal set; }` properties, so the C# call syntax was always
  property access, not a raw field read; the C++ port just needed to catch up to
  `cna`'s current convention. Updated all ~23 accesses in
  `samples/InputReporter/src/InputReporterGame.hpp` to `getXxxProperty()`. Verified
  live: `InputReporter_cna_samples` builds standalone with 0 errors, and the full
  `cmake --build cmake-build-debug -j$(nproc)` (all targets) now also succeeds with 0
  errors — this was the last known build regression in the repo.

---

## 4. Current blocker / main problem

There is no single failing build/test blocking the whole repo (the last known one,
InputReporter, was fixed this session — see section 3). The project has run out of
samples portable by routine effort; what remains needs one of:

**a) A scope decision (not a technical blocker):** AccelerometerSample (#084) and
TiltPerspective (#107) are the only two remaining "Deferred — Phone Hardware" samples
that CNA can technically support (real `Accelerometer` API) — but both originals are
Windows-Phone-only projects with **zero** non-phone input code to reuse (confirmed: no
`#if WINDOWS_PHONE`/non-phone branch exists in either sample's C#, a single `.csproj`
per sample). Porting either means inventing a full keyboard-tilt-emulation control
scheme from scratch — a bigger design commitment than any prior sensor port in this
repo (Yacht/SnowShovel/Bounce/Orientation all just un-`#if`'d an existing branch).
Needs a go/no-go decision before starting.

**b) Real CNA engine work, not started:** All remaining Phase 3 samples (3D custom
shader effects) and most of Phase 4 (skeletal animation: SkinningSample,
CustomModelAnimation, SkinnedModelExtensions, CPUSkinning) are blocked on:
  - An HLSL→GLSL shader translation workflow + `.shader.json` tooling (DEFERRED.md item
    11, effort XL). No tooling exists yet.
  - `AnimationClip`/`Keyframe`/`AnimationPlayer` types + `.model.json` schema
    extensions for per-vertex bone weights and keyframe data (item 13, effort L/XL).
    Confirmed via grep: zero matches for these types anywhere in `cna/include` or
    `cna/src`.

**c) (RESOLVED this session)** `InputReporter`'s full-repo build failure — see section 3.

**d) Environment flakiness affecting verification, not the code itself:** see section 5.

---

## 5. Known bugs and limitations

- **CONFIRMED BUG, fixed this session** — `InputReporter` failed a full-repo build
  (section 4c/3): direct `GamePadCapabilities` field reads switched to
  `getXxxProperty()` calls.
- **CONFIRMED BUG, fixed this session** — Orientation's `O` orientation-cycle had a
  visible no-op half the time (section 3).
- **CONFIRMED, real desktop limitation (not a CNA bug)** — `xdotool` keyboard/mouse
  input intermittently fails to reach sample windows on this shared, actively-used
  development desktop, even when `xdotool getactivewindow` reports the correct window
  focused. Sample processes/windows have also been observed to close unexpectedly
  mid-test on this same desktop. Always re-verify focus **immediately** before sending
  input (focus can flip away between the check and the send); don't conclude "no
  visible effect" means a code bug without first checking the sample's own state (e.g.
  a lock flag toggled by an incidental stray click) — confirmed to have caused a false
  bug report earlier this session (Orientation). Fall back to build success + an
  idle-render screenshot when live interaction can't be confirmed.
- **CONFIRMED, framework gap** — CameraShake has a near-plane clipping bug in the
  EasyGL backend (a white stripe where geometry should render); not yet root-caused.
  Exact file/function not yet identified — needs investigation first.
- **CONFIRMED, framework gap** — the Vulkan backend discards the first of two
  `SpriteBatch.Begin()/End()` blocks issued in the same frame. The default EasyGL
  backend is unaffected; several samples (e.g. NinjAcademy, HoneycombRush) rely on
  multiple per-frame `SpriteBatch` instances and would need this fixed to run
  correctly on Vulkan.
- **INCOMPLETE** — SplitScreen (#076) and TankOnHeightmap (#074) need per-mesh
  `ModelBone` support in CNA's `.model.json` reader (`ModelTypeReader::Read()` in
  `ContentManager.cpp` currently only ever builds one flat "Root" bone for the whole
  model) — full write-up in `samples/SplitScreen/missing.md`. The user intends to
  implement this directly in `cna`.
- **INCOMPLETE** — NGSMSample (#075) and NetRumble (#062) need
  `Microsoft.Xna.Framework.Net`/`GamerServices`, not implemented in CNA.
- **NEEDS VERIFICATION** — RolePlayingGame's (#070) input-driven playthrough (walk into
  a portal/chest/NPC, trigger and resolve combat) was only exercised via a temporary
  debug auto-trigger during porting, not real keyboard input, due to the `xdotool`
  issue above.
- **NEEDS VERIFICATION** — real hardware accelerometer shake/tilt (Yacht, SnowShovel)
  has never been tested on a device with a physical sensor; only the keyboard/gamepad
  fallback path has been exercised, on this sensor-less desktop.
- **NEEDS VERIFICATION** — UISample's `HighScoreScreen` vertical drag-scroll could not
  be synthetically drag-tested (an `xdotool` held-mouse-button position freeze specific
  to this desktop's WM/compositor); code-reviewed line by line with no bug found, but
  never live-confirmed.

---

## 6. Architecture notes

- **Sample layout:** `samples/<Name>/{src/,Content/,missing.md,<Name>.htm,CMakeLists.txt}`.
  `src/` is header-only except a single `src/Program.cpp` containing `main()`.
  `CONTENT_DIR` in each sample's `cna_add_sample()` call is mandatory for any sample
  loading assets (including the F1 `help.png`) — the binary aborts at startup without
  it. Content is copied next to the built binary via a `POST_BUILD` step, so a sample
  must be **run from its own binary directory**, not the repo root.
- **CNA property pattern:** CNA uses a `DEF_PROP` macro generating
  `getXxxProperty()`/`setXxxProperty()` methods, not public members or real C#-style
  properties. See CLAUDE.md's translation table for common cases (`Viewport.Width` →
  `getViewportProperty().getWidthProperty()`, etc.). Every `Game` subclass must
  implement `GetTypeName()`.
- **GameComponent lifetime invariant (do not break):** a `shared_ptr`-owning
  "screen"/"parent" object with synchronous removal (e.g. `TransitionOffTime=0`) that
  itself owns many `GameComponent`s must never be destroyed synchronously from inside
  `Game::Update()`'s/`Draw()`'s own component iteration — `Game` iterates a snapshot of
  `Game.Components` taken at the top of the frame, and synchronous mid-iteration
  destruction leaves dangling entries in that snapshot (XNA's GC never frees anything
  synchronously, so the original C# has no equivalent hazard). Established fix pattern:
  defer the actual `shared_ptr` release to the start of the next frame's `Update()` —
  used in HoneycombRush/NinjAcademy/CardsStarterKit/RolePlayingGame's `ScreenManager`s
  and `CardsFramework::CardsGame`.
- **Input fallback patterns (pick the one matching the interaction shape, don't invent
  a new one):** single discrete tap → synthesize one `GestureType::Tap` on a mouse
  left-click rising edge, either by pushing into a private gesture list (DynamicMenu)
  or via the real `TouchPanel::EnqueueGesture()` (NinjAcademy, Orientation — preferred,
  since it requires zero changes to code that already polls
  `TouchPanel::ReadGesture()`); continuous drag → track mouse position while a button
  is held; no rotation sensor → a keyboard key (`O`, established in DynamicMenu and
  reused in Orientation) manually resizes the back buffer to simulate physical device
  rotation.
- **Boundaries not to cross:** no shared `samples/common/` library, even between
  structurally similar samples. Do not use CNA's NOXNA `SetBlendEnabled`/
  `SetDepthTestEnabled`/`SetDepthWriteEnabled` helpers — use the real XNA state objects
  (`BlendState`, `DepthStencilState`, `RasterizerState`) directly. Do not switch
  `ScreenManager` screen ownership from `shared_ptr` to raw pointers.
- **CNA/sharp-runtime boundary:** these are separate, independently-developed sibling
  repos consumed via `add_subdirectory`. Never assume their working tree is clean;
  never edit them without first confirming scope with the user, since other work may be
  happening there concurrently.

---

## 7. Useful commands

Configure (from repo root; only needed once or after a CMakeLists.txt change):
```
cmake -S . -B cmake-build-debug
```
If CMake complains about the vendored ENet dependency's minimum version:
```
cmake -S . -B cmake-build-debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

Build one sample (target name is `<SampleDirName>_cna_samples`):
```
cmake --build cmake-build-debug --target Orientation_cna_samples -j$(nproc)
```

Build everything (all targets, confirmed passing this session):
```
cmake --build cmake-build-debug -j$(nproc)
```

Run a sample (must `cd` into its own binary directory first — `Content/` is copied
there, not to the repo root):
```
cd cmake-build-debug/samples/Orientation
SDL_VIDEODRIVER=x11 ./Orientation_cna_samples
```

Generate a font asset:
```
python3 tools/make_font.py /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf 14 samples/<Name>/Content/font
```

Generate the mandatory F1 help overlay:
```
python3 tools/gen_help_png.py samples/<Name>/<Name>.htm samples/<Name>/Content/help.png
```

No lint/format command and no automated test suite are configured in this repo.

---

## 8. Next smallest tasks

1. **Investigate CameraShake's near-plane clipping bug in the EasyGL backend.**
   - Goal: clip `w<0` vertices the way DirectX does, so the ground/tank fully render.
   - Files: likely `cna/src/.../EasyGL/EasyGLGraphicsBackend.cpp` (clipping/projection
     path) — exact location not yet confirmed, needs investigation first.
   - Verify: run `./cmake-build-debug/samples/CameraShake/CameraShake_cna_samples`,
     confirm the white stripe is gone.

2. **Fix the Vulkan multiple-SpriteBatch-per-frame bug.**
   - Goal: a second `Begin()/End()` in the same frame must not discard the first.
   - Files: `cna/src/.../Vulkan/VulkanGraphicsBackend.cpp`.
   - Verify: run GameStateManagement or CatapultWars on the Vulkan backend; confirm all
     layers draw (not just the last `Begin/End` block's sprites).

3. **Decide the scope for AccelerometerSample (#084) / TiltPerspective (#107), then
   port if approved.**
   - Goal: get an explicit go/no-go from the user on inventing a keyboard-tilt fallback
     (see section 4a) before spending the effort.
   - Files (if approved): new `samples/AccelerometerSample/`, `samples/TiltPerspective/`.
   - Verify: `cmake --build cmake-build-debug --target AccelerometerSample_cna_samples`.

4. **Run a controlled interactive pass on RolePlayingGame (#070).**
   - Goal: confirm a real keyboard-driven playthrough — walk into a portal/chest/NPC,
     trigger and resolve one combat — once `xdotool`/input reliability is confirmed.
   - Files: `samples/RolePlayingGame/src/Session/Session.hpp`, `CombatEngine`-related
     headers (read-only verification, not expected to need code changes).
   - Verify: manual playthrough with screenshot evidence at each step.

5. **(User-owned, tracked here for visibility)** Add per-mesh `ModelBone` support to
   CNA's `.model.json` reader.
   - Goal: unblock SplitScreen (#076), TankOnHeightmap (#074), SimpleAnimation (#050).
   - Files: `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`'s
     `ModelTypeReader::Read()`, plus a new `ModelMesh` parent-bone setter.
   - Verify: `samples/SplitScreen/` (currently only `missing.md` + `.htm`, no source)
     can be built and renders independently-posed tank parts.

6. **Audit all 45 already-ported samples' `missing.md` for completeness.**
   - Goal: every sample — ported or not — should fully document what it's missing vs.
     CNA (not just what was known/written at port time), so gaps can be picked up and
     fixed in a `cna` session without re-deriving them from scratch.
   - Files: `samples/<Name>/missing.md` for all 45 done samples. Compare each against
     its XNA C# original (`/rv/tmp/XNAGameStudio/Samples/...`) and the current `cna`
     source, the same way this session's 36 placeholder `missing.md` files were
     produced (direct grep/read, not assumption).
   - Verify: spot-check a handful of the updated files cite concrete file:line
     evidence rather than restating what CLAUDE.md already says is a known pattern.

---

## 9. Do not do yet

- **Do not start a Phase 3 (3D shader) sample** without first building an HLSL→GLSL
  `.shader.json` workflow in `cna` (DEFERRED.md item 11) — no tooling exists yet.
- **Do not start a skeletal-animation Phase 4 sample** (SkinningSample,
  CustomModelAnimation, SkinnedModelExtensions, CPUSkinning) without
  `AnimationClip`/`Keyframe`/`AnimationPlayer` existing in `cna` (item 13).
- **Do not invent a keyboard-tilt input scheme for AccelerometerSample/TiltPerspective**
  without an explicit scope decision from the user first (section 8, item 3).
- **Do not add a shared `samples/common/` library**, even where two samples' code looks
  structurally similar.
- **Do not edit `cna`/`sharp-runtime` source** without confirming scope with the user
  first — they are independently active sibling repos.
- **Do not hammer `xdotool` input** without re-verifying window focus immediately
  beforehand, and do not conclude "no visible effect" means a code bug without first
  checking the sample's own state (section 5).
- **No broad refactors or unrelated cleanup** while the CameraShake/Vulkan bugs
  (section 8, items 1–2) are open.
- **Do not regenerate existing font atlases or `.model.json` assets** unless there is a
  confirmed rendering bug — regenerating is cheap but pointless churn otherwise.
- **Do not add a directory under `samples/` for anything listed in `ignored.md`** —
  that list is the result of a deliberate scope decision (not XNA 4.0, not a runnable
  `Game`, redundant training-kit duplicate, or tied to a platform CNA won't target).
  If a reason ever stops applying, remove it from `ignored.md` and add it to `PLAN.md`
  properly rather than silently creating the directory.

---

## 10. Resume prompt

```
Read NEXT.md first to understand the current project state.
Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not start any task not listed in section 8.
Make one small, verified improvement, run the verification command listed for that
task, and confirm it actually passes.
After finishing, update NEXT.md (status, recent changes, next tasks) to match reality.
```
