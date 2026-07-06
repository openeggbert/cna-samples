# NEXT.md

## 1. Project summary

**What this is:** C++ ports of the official Microsoft XNA Game Studio 4.0 sample
collection, running on **CNA** (`../cna`) — a C++ reimplementation of the XNA 4.0
API built on SDL3, itself built on **sharp-runtime** (`../sharp-runtime`), a C++
port of relevant .NET BCL types. `cna` and `sharp-runtime` are independent sibling
git repositories consumed via `add_subdirectory`, not submodules of this repo.

**Main goal:** Port the applicable XNA 4.0 desktop samples (`PLAN.md` catalogs all
153 directories in the upstream XNA Game Studio archive and classifies each one) to
CNA C++, preserving the original class hierarchy and naming
(`Microsoft::Xna::Framework::*`). The ported samples double as integration tests for
CNA and as a migration reference for anyone porting XNA/MonoGame code to CNA.

**Current phase:** 48 samples are fully ported and wired into the root
`CMakeLists.txt`. 10 more are confirmed unblocked (no remaining CNA gap) and ready
to port. 28 placeholder directories exist for samples still genuinely blocked on
real CNA engine work (custom shaders, skeletal animation, one content-pipeline
gap). 67 catalogued directories are permanently out of scope and listed in
`ignored.md` (not XNA 4.0, not a runnable `Game`, redundant duplicates, or tied to
a platform CNA won't target). See `PLAN.md`'s Sample Count Summary table for exact
per-category counts.

**Important architectural decisions:**
- One executable per sample; no shared sample library. Each `samples/<Name>/`
  directory is fully self-contained (`src/` header-only except one `Program.cpp`,
  `Content/`, `missing.md`, a verbatim copy of the original `.htm` doc) — even
  where two samples' code ends up structurally similar.
- Assets: `Load<Texture2D>` → PNG; `Load<SoundEffect>`/`Load<Song>` → WAV/OGG;
  `Load<SpriteFont>` → `.font.json` + PNG atlas (`tools/make_font.py`);
  `Load<Model>` → `.model.json` (static/rigid single-bone models only — see
  section 5/6). XNA `.xnb` is never supported. Custom XML content files are
  hand-translated to C++ construction code in small numbers; the one exception is
  RolePlayingGame's 281 data files, which needed a real runtime XML loader
  instead.
- CNA is consumed via `add_subdirectory(../cna)`, so building any sample also
  rebuilds CNA (and transitively sharp-runtime) if their sources changed. Never
  assume `cna`/`sharp-runtime`'s working tree is clean, and never edit them
  without confirming scope with the user first — other work may be happening
  there concurrently (confirmed: this machine runs many concurrent Claude Code
  sessions against these same repos).
- Default graphics backend is **EasyGL** (OpenGL ES); a Vulkan backend also exists
  but has a known bug (section 5).
- Framework-level gaps found while porting a sample get fixed in `cna`/
  `sharp-runtime` directly (after confirming scope with the user) — never worked
  around silently inside a sample. Every workaround applied instead must be
  documented in that sample's `missing.md`.

---

## 2. Current status

### Build
As of commit `eee6769` on `develop` (pushed), a full build succeeded with 0
errors and 0 warnings:
```
cmake --build cmake-build-debug -j$(nproc)
```
This produced 94 targets. Not re-verified in the current turn (no build was run
this turn per explicit instruction) — treat as "last known good" as of that
commit, not a live guarantee.

### Tests
No automated test suite exists in this repo. Each sample is its own manual/visual
verification unit: build it, run it under `SDL_VIDEODRIVER=x11`, and (when
synthetic input can be verified — see section 5) interact with it or compare a
screenshot.

### CLI / tools available
- `tools/make_font.py <font.ttf> <size_px> <Content/FontName>` — generates a
  `.font.json` + atlas PNG SpriteFont asset.
- `tools/gen_help_png.py <Sample.htm> <Content/help.png>` — extracts a sample's
  own `.htm` "Sample Controls" table into the mandatory F1 help-overlay PNG. Note:
  hardcodes column index 1 for the keyboard control — samples whose table has
  more columns (e.g. a Windows-Phone column first) need a one-off variant script
  to pick the right column (done once for MicrophoneEcho; not generalized).
- `tools/obj2model.py`, `tools/fbx_ascii2model.py` — convert static, single-bone
  `.obj`/`.fbx` models to CNA's `.model.json` format.

### Recently implemented / working
- `Microsoft::Xna::Framework::Audio::Microphone` + `DynamicSoundEffectInstance`
  (MicrophoneEcho, #098) — capture-device enumeration, `BufferReady`/`GetData()`,
  loopback playback. Live-verified: a real capture device was found on this
  machine and the waveform render pipeline works.
- `Microsoft::Xna::Framework::Net::NetworkSession` + `GamerServices` (basic path)
  (ClientServerSample, #091) — session creation, `GamerJoined` handling, packet
  send/receive, all live-verified for a solo (single-gamer) session. Three real
  gaps found and worked around at the sample level (section 5).
- `VertexPositionNormalTexture` lit rendering (`BasicEffect.EnableDefaultLighting`)
  in the EasyGL backend — proven by a live-run integration test
  (`cna_test_easygl_basiceffect_combinations`, case "(e)", exit 0) and by porting
  CustomModelClass (#052), which builds and runs against it.
- `CNA_Net`/`CNA_GamerServices` CMake wiring in this repo (previously the targets
  existed in `cna` but were never enabled/linked here — see section 3).

### Known NOT working
- Visual correctness of tank-model-based lit rendering: CustomModelClass (#052)
  and the pre-existing CameraShake both render `tank.model.json` as a thin
  diagonal line instead of a recognizable model — a real, unfixed EasyGL
  near-plane clipping bug (section 4/5), not specific to either sample.
- Multi-gamer `NetworkSession` state routing: `NetworkGamer.Id` is a hardcoded
  stub (always `0`), so `FindGamerById()` always resolves to the first gamer —
  correct for solo sessions (verified), wrong once a second gamer joins
  (unverified — no second instance was tested).
- Interactive keyboard/mouse input verification via `xdotool` on this shared
  development desktop is unreliable (section 5) — blocks live-testing new
  samples' input handling, not the samples' own code.

---

## 3. Recent changes

Most recent session (2026-07-06), in order:
- Fixed `InputReporter`'s full-repo build failure: it read `GamePadCapabilities`
  fields directly (`cap.HasLeftStickButton`); upstream `cna` made them private
  behind `getXxxProperty()`. Updated all ~23 accesses in
  `samples/InputReporter/src/InputReporterGame.hpp`.
- Added `ignored.md`: lists all 67 catalogued sample directories that will never
  get a `samples/` directory, each with a one-line reason. `PLAN.md`'s old
  per-category tables for these were collapsed into a single pointer.
- Added 36 placeholder directories (`<Name>.htm` + `missing.md`, no source) for
  every remaining XNA 4.0 sample judged worth tracking once CNA can do more.
  Root `CMakeLists.txt` got matching commented-out `add_subdirectory()` lines.
- Added `DEFERRED.md` items #16–18 (microphone capture, multiplayer networking,
  content-pipeline processor extensibility).
- Audited all 45 (at the time) already-ported samples' `missing.md` against
  their C# originals and current `cna` source. **Incident:** one sub-agent
  misread `git status` (many concurrent Claude sessions touch this repo — normal)
  and ran `git checkout` on 31 files it didn't own, discarding other batches'
  completed work. Caught, confirmed placeholder dirs were untouched, fully
  re-ran the 6 affected batches.
- Corrected DEFERRED.md items #16, #17, and (pre-existing) #5 after finding they
  were stale — live source/build checks showed `cna` already resolved them
  (merged 2026-07-04, two days before item #16/#17 were written this session).
  This unblocked 13 samples outright: MicrophoneEcho (#098), ClientServerSample
  (#091), NetworkPrediction (#100), PeerToPeer (#103) (item #17), and LensFlare
  (#041), Graphics3D (#046), PickingSample (#047), TrianglePicking (#048),
  HeightmapCollision (#049), CustomModelClass (#052), InverseKinematics (#057),
  ChaseCamera (#058), MarbleMaze (#061) (item #5). NetRumble (#062) went from
  double- to single-blocked.
- Ported **MicrophoneEcho (#098)**. Builds 0 warnings; live-verified via
  screenshot.
- Ported **ClientServerSample (#091)**. Surfaced and worked around 3 new CNA gaps
  (DEFERRED.md items #19–21 — see section 5) and fixed a `cna-samples`-side
  build-wiring gap: `CNA_Net`/`CNA_GamerServices` are separate CMake targets in
  `cna`, gated behind `CNA_ENABLE_NET` (off by default) and never linked by
  `cna_add_sample()`. Fixed in this repo's root `CMakeLists.txt` (added
  `set(CNA_ENABLE_NET ON ...)`) and `cmake/SampleHelpers.cmake` (added
  `if(TARGET CNA_Net) target_link_libraries(... CNA_Net) endif()`). Live-verified:
  session creates, tank spawns, renders correctly, no crash over 8+ seconds.
  `JoinSession()`/2-machine path not verified.
- Ported **CustomModelClass (#052)**, using stock `Model`/`BasicEffect` instead of
  the C# original's own `CustomModel` class (CNA has no build-time custom-
  `ContentProcessor` extensibility — item #18). Builds/runs clean, but rendering
  hit the near-plane clipping bug described in section 4 — confirmed via a
  side-by-side screenshot with CameraShake (identical artifact) that this is
  pre-existing, not a new regression.
- Rewrote this file (`NEXT.md`) to the current 10-section template.

Commits this session (newest first): `eee6769`, `155bc18`, `d9a2baf`, `580283c`,
`4dd7ceb`, `d747356`. All pushed to `develop`.

---

## 4. Current blocker / main problem

**There is no failing build or test blocking the whole project right now.** The
most significant *open, concrete problem* is a rendering bug:

- **Exact symptom:** a `tank.model.json`-based `Model`, drawn with
  `BasicEffect.EnableDefaultLighting()` through a perspective camera at a
  moderate distance (~500–1000 XNA units), renders as a thin diagonal
  line/dashes instead of a recognizable 3D model.
- **Failing command (to reproduce):**
  ```
  cd cmake-build-debug/samples/CameraShake
  SDL_VIDEODRIVER=x11 ./CameraShake_cna_samples
  ```
  (or `cmake-build-debug/samples/CustomModelClass/CustomModelClass_cna_samples`
  — same artifact, same asset.)
- **No failing automated test** — there is no test suite; this was found by
  screenshot comparison.
- **Affected files/modules:** almost certainly
  `cna/src/CNA/Internal/Backends/EasyGL/EasyGLGraphicsBackend.cpp` (clipping/
  projection path) — not yet confirmed by direct inspection.
- **Suspected cause:** near-plane (`w<0`) vertex clipping in the EasyGL backend
  does not match DirectX/real-XNA behavior, so triangles crossing the near plane
  degenerate instead of being clipped correctly.
- **What has been tried:** confirmed (2026-07-06) via a side-by-side screenshot
  that CameraShake and CustomModelClass show the *identical* artifact in the
  *identical* screen position — this rules out a per-sample bug and confirms a
  shared framework cause. Not yet root-caused inside `cna` itself; no fix
  attempted.
- **Why it matters now:** 8 more samples (LensFlare, Graphics3D, PickingSample,
  TrianglePicking, HeightmapCollision, InverseKinematics, ChaseCamera,
  MarbleMaze) are otherwise unblocked and portable, but **should not be assumed
  to render correctly** just because they build — each needs its own screenshot
  check for this same artifact once ported.

Secondary, lower-urgency items (not blocking, just open):
- A product/scope decision is needed before porting AccelerometerSample (#084)/
  TiltPerspective (#107) (inventing a keyboard-tilt scheme from scratch) or the
  5 Avatar samples now reopened by `cna`'s new substitute-body rendering path
  (section 8, items 6–7).
- The Vulkan backend's multi-`SpriteBatch`-per-frame bug (section 5) is separate
  from the above and only affects the non-default Vulkan backend.

---

## 5. Known bugs and limitations

- **CONFIRMED BUG, unfixed** — EasyGL near-plane clipping renders certain
  `Model`-based geometry (confirmed: `tank.model.json`, at both CameraShake's and
  CustomModelClass's camera distances) as a degenerate thin line instead of the
  model. See section 4 for full detail. Likely affects other models/samples too;
  not yet characterized beyond the tank asset.
- **CONFIRMED BUG, workaround applied (ClientServerSample, #091)** —
  `GamerServicesDispatcher::Update()` in `cna` is a no-op, so
  `NetworkSession::Create`/`Find`/`Join`'s synchronous busy-wait loop never
  completes if a `GamerServicesComponent` has been added (which every C#
  original does). Sample-level workaround: don't add one — DEFERRED.md item #19.
- **CONFIRMED BUG, partially worked around (ClientServerSample, #091)** —
  `NetworkGamer.IsHost` and `.Id` are hardcoded stub constants (always
  `true`/`0`), not real per-instance state. `IsHost` worked around with a
  locally-tracked flag; `Id`/`FindGamerById` is not — multi-gamer sessions will
  misroute state to the first gamer. DEFERRED.md item #20.
- **CONFIRMED BUG, workaround applied (ClientServerSample, #091)** — the initial
  `GamerJoined` event is queued for the next `Update()` instead of raised
  synchronously during `Create()`/`Join()` like real XNA. Workaround: call
  `networkSession_->Update()` once right after subscribing to events.
  DEFERRED.md item #21.
- **CONFIRMED, framework gap, unfixed** — the Vulkan backend discards the first
  of two `SpriteBatch.Begin()/End()` blocks issued in the same frame. EasyGL
  (default) is unaffected. Several samples rely on multiple per-frame
  `SpriteBatch` instances and would need this fixed to run correctly on Vulkan.
- **INCOMPLETE** — SplitScreen (#076), TankOnHeightmap (#074), SimpleAnimation
  (#050) need per-mesh `ModelBone` support in CNA's `.model.json` reader
  (`ModelTypeReader::Read()` in `ContentManager.cpp` currently only ever builds
  one flat "Root" bone). User intends to implement this directly in `cna`.
- **INCOMPLETE** — NGSMSample's single-player path and NetRumble both still need
  real gameplay content/shaders respectively even though the underlying
  networking API now exists.
- **NEEDS VERIFICATION** — ClientServerSample's `JoinSession()` and actual
  2-machine LAN discovery/join were never tested (would need two running
  instances).
- **NEEDS VERIFICATION** — RolePlayingGame's (#070) input-driven playthrough
  (portal/chest/NPC/combat) was only exercised via a temporary debug
  auto-trigger, not real keyboard input.
- **NEEDS VERIFICATION** — real hardware accelerometer shake/tilt (Yacht,
  SnowShovel) has never been tested on a device with a physical sensor.
- **NEEDS VERIFICATION** — UISample's `HighScoreScreen` vertical drag-scroll was
  never live-drag-tested (code-reviewed only).
- **ENVIRONMENT LIMITATION, not a code bug** — `xdotool` keyboard/mouse input on
  this shared development desktop intermittently fails to reach sample windows
  even when focus is confirmed via `xdotool getactivewindow` immediately before
  sending. Screenshot tooling (`import`) has also intermittently failed with raw
  X-server errors (`BadMatch`/`X_GetImage`) under heavy concurrent load, unrelated
  to any sample's code. When live input can't be confirmed, this repo's
  established fallback is a temporary, clearly-commented debug auto-trigger
  (removed before commit) plus a screenshot, not blind trust that "no visible
  change" means a bug.
- **RISKY ASSUMPTION, worth rechecking periodically** — DEFERRED.md blockers can
  go stale silently, because `cna` is under active, concurrent development by
  other sessions/branches. A blocker confirmed accurate today is not guaranteed
  accurate next session; re-verify live (grep `cna`'s current source, or build+
  run an existing test) before treating any DEFERRED.md item as gating a real
  decision.

---

## 6. Architecture notes

- **Sample layout:** `samples/<Name>/{src/,Content/,missing.md,<Name>.htm,
  CMakeLists.txt}`. `src/` is header-only except a single `src/Program.cpp`
  containing `main()`. `CONTENT_DIR` in each sample's `cna_add_sample()` call is
  mandatory for any sample loading assets (including the F1 `help.png`) — the
  binary aborts at startup without it. Content is copied next to the built
  binary via a `POST_BUILD` step, so a sample must be **run from its own binary
  directory**, not the repo root.
- **CNA property pattern:** CNA uses a `DEF_PROP` macro generating
  `getXxxProperty()`/`setXxxProperty()` methods, not public members or real
  C#-style properties (see `CLAUDE.md`'s translation table). Every `Game`
  subclass must implement `GetTypeName()`.
- **CMake module split in `cna`:** the main `CNA` static-lib target excludes
  `Microsoft::Xna::Framework::{GamerServices,Net}` sources entirely; those live
  in separate `CNA_GamerServices`/`CNA_Net` targets gated behind the
  `CNA_ENABLE_NET` option (this repo now sets it `ON`; see `cmake/
  SampleHelpers.cmake` for the matching `target_link_libraries` addition). Any
  future sample needing `Net`/`GamerServices` types gets this automatically.
- **GameComponent lifetime invariant (do not break):** a `shared_ptr`-owning
  "screen"/"parent" object that itself owns many `GameComponent`s must never be
  destroyed synchronously from inside `Game::Update()`'s/`Draw()`'s own
  component iteration (`Game` iterates a snapshot of `Game.Components` taken at
  the top of the frame). Established fix: defer the actual `shared_ptr` release
  to the start of the next frame's `Update()` — used in HoneycombRush/
  NinjAcademy/CardsStarterKit/RolePlayingGame's `ScreenManager`s.
- **Input fallback patterns (pick the one matching the interaction shape, don't
  invent a new one):** single discrete tap → synthesize a `GestureType::Tap` on
  a mouse left-click rising edge via `TouchPanel::EnqueueGesture()` (preferred);
  continuous drag → track mouse position while a button is held; no rotation
  sensor → a keyboard key manually resizes the back buffer.
- **NetworkSession usage pattern (established by ClientServerSample, #091):**
  do **not** add a `GamerServicesComponent` (hangs `Create`/`Find`/`Join` — see
  section 5, item #19); track "am I the host" with a locally-scoped bool set at
  the `Create()`/`Join()` call site, never via `gamer.IsHost`/
  `networkSession.IsHost`; call `networkSession_->Update()` once immediately
  after subscribing to session events, to flush the initial `GamerJoined` event
  before the first real per-frame `Update()` reads gamer `Tag` state.
- **Boundaries not to cross:** no shared `samples/common/` library, even between
  structurally similar samples. Do not use CNA's NOXNA `SetBlendEnabled`/
  `SetDepthTestEnabled`/`SetDepthWriteEnabled` helpers — use the real XNA state
  objects (`BlendState`, `DepthStencilState`, `RasterizerState`) directly. Do not
  switch `ScreenManager` screen ownership from `shared_ptr` to raw pointers.
- **`cna`/`sharp-runtime` boundary:** separate, independently-developed sibling
  repos consumed via `add_subdirectory`. Never assume their working tree is
  clean; never edit them without confirming scope with the user first.

---

## 7. Useful commands

Configure (from repo root; only needed once or after a `CMakeLists.txt` change):
```
cmake -S . -B cmake-build-debug
```
If CMake complains about the vendored ENet dependency's minimum version:
```
cmake -S . -B cmake-build-debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

Build one sample (target name is `<SampleDirName>_cna_samples`):
```
cmake --build cmake-build-debug --target CustomModelClass_cna_samples -j$(nproc)
```

Build everything:
```
cmake --build cmake-build-debug -j$(nproc)
```

Run a sample (must `cd` into its own binary directory first — `Content/` is
copied there, not to the repo root):
```
cd cmake-build-debug/samples/CustomModelClass
SDL_VIDEODRIVER=x11 ./CustomModelClass_cna_samples
```

Reproduce the current main problem (section 4) — compare against CameraShake,
which shows the identical artifact:
```
cd cmake-build-debug/samples/CameraShake
SDL_VIDEODRIVER=x11 ./CameraShake_cna_samples
```

Generate a font asset:
```
python3 tools/make_font.py /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf 14 samples/<Name>/Content/font
```

Generate the mandatory F1 help overlay:
```
python3 tools/gen_help_png.py samples/<Name>/<Name>.htm samples/<Name>/Content/help.png
```

No lint/format command and no automated test suite are configured in this repo.

---

## 8. Next smallest tasks

1. **Screenshot-verify one of the 8 remaining unblocked lighting samples for the
   near-plane clipping bug before/while porting it — recommended: LensFlare
   (#041), the other simplest pick.**
   - Goal: port it using stock `Model`/`BasicEffect` (same pattern as
     CustomModelClass), then confirm via screenshot whether it hits the same
     rendering bug as CameraShake/CustomModelClass, or renders correctly (its
     terrain model may differ in scale/geometry from the tank).
   - Files: new `samples/LensFlare/src/`; see `samples/LensFlare/missing.md`.
   - Verify: `cmake --build cmake-build-debug --target LensFlare_cna_samples`,
     then run under `SDL_VIDEODRIVER=x11` and screenshot.

2. **Investigate the EasyGL near-plane clipping bug itself (section 4).**
   - Goal: clip `w<0` vertices correctly so the tank/terrain models render fully
     instead of degenerating to a thin line.
   - Files: likely `cna/src/CNA/Internal/Backends/EasyGL/
     EasyGLGraphicsBackend.cpp` (clipping/projection path) — exact location not
     yet confirmed.
   - Verify: run `CameraShake_cna_samples` and `CustomModelClass_cna_samples`
     under `SDL_VIDEODRIVER=x11`; confirm the tank/ground are fully visible in a
     screenshot, not a thin line.

3. **Port NetworkPrediction (#100) or PeerToPeer (#103).**
   - Goal: apply the same 3 workarounds ClientServerSample needed (no
     `GamerServicesComponent`; local host-tracking bool; one extra
     `Update()` call after hooking events — section 6) and confirm they're
     still needed/sufficient for these two samples' own `Update()` loop shape.
   - Files: new `samples/NetworkPrediction/src/` or `samples/PeerToPeer/src/`;
     read `samples/ClientServerSample/missing.md` and DEFERRED.md items #19–21
     first.
   - Verify: `cmake --build cmake-build-debug --target NetworkPrediction_cna_samples`
     (or `PeerToPeer_cna_samples`); screenshot the menu + a triggered session
     the way ClientServerSample was verified.

4. **Fix the Vulkan multiple-SpriteBatch-per-frame bug.**
   - Goal: a second `Begin()/End()` in the same frame must not discard the
     first.
   - Files: `cna/src/.../Vulkan/VulkanGraphicsBackend.cpp`.
   - Verify: run GameStateManagement or CatapultWars on the Vulkan backend;
     confirm all layers draw.

5. **Decide the scope for AccelerometerSample (#084)/TiltPerspective (#107).**
   - Goal: get an explicit go/no-go from the user on inventing a keyboard-tilt
     fallback from scratch (neither original has any non-phone code to reuse)
     before spending the effort.
   - Files (if approved): new `samples/AccelerometerSample/src/`,
     `samples/TiltPerspective/src/`.
   - Verify: `cmake --build cmake-build-debug --target AccelerometerSample_cna_samples`.

6. **Decide whether to port any of the 5 now-reopened Avatar samples** (#085,
   #086, #087, #094, #101) onto `cna`'s new `AvatarRenderer::
   EnableRealRenderingEXT` substitute-body path.
   - Goal: get an explicit go/no-go from the user before spending effort — this
     is a product decision (substitute art, not the real Xbox Avatar look), not
     a technical blocker.
   - Files: none yet.
   - Verify: N/A until a decision is made.

7. **(User-owned, tracked for visibility) Add per-mesh `ModelBone` support to
   CNA's `.model.json` reader.**
   - Goal: unblock SplitScreen (#076), TankOnHeightmap (#074), SimpleAnimation
     (#050).
   - Files: `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`'s
     `ModelTypeReader::Read()`, plus a new `ModelMesh` parent-bone setter.
   - Verify: `samples/SplitScreen/` (currently only `missing.md` + `.htm`) can
     be built and renders independently-posed tank parts.

---

## 9. Do not do yet

- **Do not start a custom-`.fx`-shader sample** (BloomSample, DistortionSample,
  NonPhotoRealistic, NormalMapping, PerPixelLighting, VertexLighting,
  ShadowMapping, BillboardSample, InstancedModel, ShatterEffect, Particles3D,
  XmlParticles, ShipGame, NetRumble) without first building an HLSL→GLSL
  `.shader.json` workflow in `cna` (DEFERRED.md item #11) — no tooling exists
  yet. This does **not** apply to the 8 remaining lit-`BasicEffect`-only samples
  (task 1/section 8) — those need no shader work.
- **Do not start a skeletal-animation sample** (SkinningSample,
  CustomModelAnimation, SkinnedModelExtensions, CPUSkinning) without
  `AnimationClip`/`Keyframe`/`AnimationPlayer` existing in `cna` (item #13).
- **Do not invent a keyboard-tilt input scheme for AccelerometerSample/
  TiltPerspective** without an explicit scope decision first (section 8, task 5).
- **Do not start porting any of the 5 reopened Avatar samples** without an
  explicit scope decision first (section 8, task 6).
- **Do not assume a newly-unblocked lighting sample renders correctly just
  because it builds** — screenshot it and check for the near-plane clipping
  artifact (section 4) first.
- **Do not add a shared `samples/common/` library**, even where two samples'
  code looks structurally similar.
- **Do not edit `cna`/`sharp-runtime` source** without confirming scope with the
  user first — they are independently active sibling repos.
- **Do not hammer `xdotool` input** without re-verifying window focus
  immediately beforehand, and do not conclude "no visible effect" means a code
  bug without first checking the sample's own state.
- **No broad refactors or unrelated cleanup** while the near-plane-clipping/
  Vulkan bugs (section 8, tasks 2/4) are open.
- **Do not regenerate existing font atlases or `.model.json` assets** unless
  there is a confirmed rendering bug — regenerating is cheap but pointless churn
  otherwise.
- **Do not add a directory under `samples/` for anything listed in
  `ignored.md`** without first removing it from `ignored.md` and adding it to
  `PLAN.md` properly (only if its exclusion reason has genuinely stopped
  applying).
- **Do not trust a DEFERRED.md blocker without re-verifying it live** if a real
  porting decision depends on it (section 5, "risky assumption").

---

## 10. Resume prompt

```
Read NEXT.md first to understand the current project state.
Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not start any task not listed in section 8.
Make one small, verified improvement, run the verification command listed for
that task, and confirm it actually passes.
After finishing, update NEXT.md (status, recent changes, next tasks) to match
reality.
```
