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

**Current phase:** 50 samples are fully ported and wired into the root
`CMakeLists.txt`. 8 more are confirmed unblocked (no remaining CNA gap) and ready
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
**A full aggregate build currently FAILS** (verified live this session,
2026-07-09) — 8 errors, all in the pre-existing `SafeArea` sample, unrelated to
this session's own changes:
```
/rv/data/development/github.com/openeggbert/cna-samples/samples/SafeArea/src/SafeAreaOverlay.hpp:47:39:
error: 'class Microsoft::Xna::Framework::Graphics::Viewport' has no member named 'x'
```
(and 7 more of the same shape, for `.x`/`.y` at lines 47/48/50/51/53/56/58).
`Viewport` no longer exposes direct `.x`/`.y` members upstream in `cna` — same
class of drift that previously broke `InputReporter`'s direct `GamePadCapabilities`
field access (section 3's history); `SafeAreaOverlay.hpp` needs the same treatment
(switch to `viewport.getXProperty()`/`getYProperty()`). **Not fixed this session**
— out of scope for the task that found it (porting LensFlare); a small, mechanical
fix for a future session (or right now, if the user wants it fixed opportunistically
— it's a ~7-line change in one file).

`LensFlare_cna_samples` itself (this session's own new target) builds cleanly with
0 errors/0 warnings, standalone and repeatedly re-verified:
```
cmake --build cmake-build-debug --target LensFlare_cna_samples -j$(nproc)
```
Last full aggregate build known to fully succeed was commit `eee6769` on
`develop` (94 targets, 0 errors/0 warnings) — no longer a live guarantee given the
`SafeArea` break above; some other sample(s) not touched by this session may also
be affected if the same `Viewport` API change hit them (not audited beyond
`SafeArea`, which is the only one the aggregate build reached before stopping).

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
- **Graphics3D (#046) ported** (2026-07-09) — `DrawableGameComponent` (touch
  buttons ported to mouse), stock `Model`/`BasicEffect` with 3 directional
  lights + specular + per-pixel-lighting toggle, sprite-sheet animation. Builds
  0 warnings, runs 6+ seconds with no crash. Converted `spaceship.fbx` (an old
  *binary* FBX 6.1 file, unreadable by `assimp`/Blender/this repo's own
  `tools/fbx_ascii2model.py`) via a one-off `ufbx`-based script → `.obj` →
  `tools/obj2model.py`. Found and worked around a real CNA component-lifecycle
  bug (`Game::DoInitialize()` wires `ComponentAdded` after calling the user's
  `Initialize()`, DEFERRED.md item #23) that segfaulted the port on its first
  frame. The spaceship model itself doesn't render — extensively isolated this
  session to the same near-plane-clipping-family bug already tracked for
  `CameraShake`/`CustomModelClass`/`LensFlare`, now confirmed to also cause full
  invisibility (not just a thin line) at longer camera distances. Also found
  `GraphicsDevice::Clear(Color)` never clears the depth buffer (DEFERRED.md item
  #24, latent, not blocking). See `samples/Graphics3D/missing.md` for the full
  account.
- **LensFlare (#041) ported** (2026-07-09) — `DrawableGameComponent`, stock
  `Model`/`BasicEffect`, and `OcclusionQuery`, all working end-to-end with no
  CNA-side API gaps for the sample's own code. Builds 0 warnings, runs 5+ seconds
  with no crash. Found and fixed a real bug in `tools/fbx_ascii2model.py` along the
  way (see below). See `samples/LensFlare/missing.md` for the full account.
- `tools/fbx_ascii2model.py` now applies each FBX mesh's baked
  `PreRotation`/`LclRotation`/`LclScaling`/`LclTranslation` node transform instead
  of silently ignoring it — `terrain.fbx` (LensFlare's asset) has a `-90,0,0`
  `PreRotation` (a Z-up → Y-up correction) that, unapplied, put the entire terrain
  outside the camera's view volume (rendered as nothing at all, not even the
  near-plane-clipping artifact). Confirmed this does **not** change `tank.fbx`'s
  already-shipped output (all its `PreRotation`s are `0,0,0`) — see DEFERRED.md
  item #6's addendum.
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
- **Full aggregate build** (`cmake --build cmake-build-debug -j$(nproc)` with no
  `--target`) — fails in `SafeArea` (see Build subsection above). Newly discovered
  this session, unrelated to LensFlare. Any single-sample `--target` build
  (including `LensFlare_cna_samples`) is unaffected.
- Visual correctness of tank-model-based lit rendering: CustomModelClass (#052)
  and the pre-existing CameraShake both render `tank.model.json` as a thin
  diagonal line instead of a recognizable model — a real, unfixed EasyGL
  near-plane clipping bug (section 4/5), not specific to either sample. **Now
  confirmed on two further, independent assets:** LensFlare's `terrain.model.json`
  shows the identical thin-line artifact; Graphics3D's spaceship (at a longer
  camera distance, ~3523 vs. ~1059 units) renders as **fully invisible** instead —
  same underlying bug family, a new/different visible symptom depending on camera
  distance (isolated via direct swap-in testing, see
  `samples/Graphics3D/missing.md`).
- **EasyGL backend never applies `BlendState.ColorWriteChannels`** (no
  `glColorMask` anywhere in `EasyGLGraphicsBackend`) — found via LensFlare's
  occlusion-query trick, which relies on `ColorWriteChannels.None` to keep its test
  polygon invisible; it renders as a solid white square instead. DEFERRED.md item
  #22 (new, not started). See `samples/LensFlare/missing.md` for the full account.
- LensFlare's glow/flare sprites (gated on the occlusion query's pixel count) were
  never observed to appear during this session's live verification — not
  root-caused; may be a symptom of the `ColorWriteChannels` gap above, or a
  separate issue. Flagged in `samples/LensFlare/missing.md`, not yet its own
  DEFERRED.md item.
- **New: `Game::DoInitialize()` wires `Components_.ComponentAdded` after calling
  the user's `Initialize()` override**, not before (real XNA/FNA does this in the
  `Game` constructor, before `Initialize()` can run) — a component added to
  `Components` from within `Initialize()` (a pattern the Graphics3D C# original
  relies on) never gets its own `Initialize()`/`LoadContent()` called, since the
  event that would trigger it isn't subscribed yet. Segfaulted Graphics3D's first
  `Draw()` until worked around. DEFERRED.md item #23 (new, not started). See
  `samples/Graphics3D/missing.md`.
- **New: `GraphicsDevice::Clear(Color)` (single-arg overload) never clears the
  depth buffer** — confirmed via direct source read; only the multi-arg
  `Clear(...)` overloads do. Affects every 3D sample in this repo (all use the
  single-arg overload), latent but not currently blocking anything observed.
  DEFERRED.md item #24 (new, not started).
- Multi-gamer `NetworkSession` state routing: `NetworkGamer.Id` is a hardcoded
  stub (always `0`), so `FindGamerById()` always resolves to the first gamer —
  correct for solo sessions (verified), wrong once a second gamer joins
  (unverified — no second instance was tested).
- Interactive keyboard/mouse input verification via `xdotool` on this shared
  development desktop is unreliable (section 5) — blocks live-testing new
  samples' input handling, not the samples' own code.

---

## 3. Recent changes

**Newest session (2026-07-09, same-day follow-up):** Ported **Graphics3D
(#046)**, picked interactively (user asked "which sample next" after LensFlare
shipped; recommended it as the leanest remaining unblocked candidate — one FBX
model, one texture, no `.x`-format blockers). Three real, separate findings
this session, on top of the port itself:

1. **`spaceship.fbx` is an old *binary* FBX (v6.1/6000)** — unreadable by
   `assimp` 5.4 and Blender 4.3.2's FBX importer (both explicitly refuse
   anything older than FBX 2011) and by this repo's own
   `tools/fbx_ascii2model.py` (ASCII-only). Worked around with a one-off
   `ufbx`-based Python script (installed in a scratch virtualenv, not added as
   a repo dependency) that bakes the mesh's node transform and writes a plain
   `.obj`, then fed that through the existing `tools/obj2model.py` — no new
   converter added to `tools/`, since this is (so far) a one-off asset format.
2. **Found and worked around a real CNA component-lifecycle bug**: this
   sample's C# original creates its 4 `Checkbox` components from inside
   `Initialize()` (not the constructor) — a pattern real XNA/FNA supports
   because `Game`'s constructor subscribes to `Components.ComponentAdded`
   immediately. `cna`'s `Game::DoInitialize()` instead subscribes that event
   *after* calling the user's `Initialize()` override, so components added
   from inside `Initialize()` never get their own `Initialize()`/
   `LoadContent()` called — segfaulted on the very first `Draw()` (dereferencing
   an unset `std::optional<SpriteBatch>`). Worked around with an explicit
   `component->Initialize()` call right after `Add()`. Filed as DEFERRED.md
   item #23 — every other sample in this repo happens to add components from
   the constructor instead, which is why this hadn't surfaced before.
3. **The spaceship model itself doesn't render.** Extensively isolated via
   direct experimentation (hand-computed NDC coordinates confirmed the math is
   correct; swapping in the already-proven `tank.model.json` through the exact
   same drawing code at the exact same ~3523-unit camera distance also
   rendered nothing; the *same* substitution at `CustomModelClass`'s own
   ~1059-unit camera distance reproduced the known thin-line artifact) — this
   is the same near-plane-clipping-family EasyGL bug already tracked for
   `CameraShake`/`CustomModelClass`/`LensFlare` (section 4), now confirmed to
   cause **full invisibility**, not just a degenerate thin line, at longer
   camera distances. Also found — while testing an unrelated hypothesis during
   this investigation — that `GraphicsDevice::Clear(Color)`'s single-argument
   overload never clears the depth buffer (real XNA's does); confirmed this is
   not what's hiding the spaceship, but it's a real, separate gap affecting
   every 3D sample in this repo. Filed as DEFERRED.md item #24.

Also confirmed live via a temporary debug auto-trigger (removed before commit):
starfield background toggle, explosion sprite-sheet animation, all 4 buttons'
icon/tint state, and the F1 help overlay (custom-written control text, since
the original `.htm` has no keyboard/gamepad table — it's touch-only) all render
correctly. Mouse substitutes for the original's touch/gesture input throughout
(drag-to-rotate, wheel-to-zoom, click-to-toggle) — not separately exercised via
synthetic input this session. Builds 0 warnings; ran 6+ seconds with no crash.
See `samples/Graphics3D/missing.md` for the complete account.

**Same-day, earlier:** Ported **LensFlare (#041)**, section 8 task 1 from
the prior session's handoff. Used `DrawableGameComponent` for `LensFlareComponent`
(matching the C# original's own component split), stock `Model`/`BasicEffect` for
the terrain, and CNA's `OcclusionQuery` for the sun-visibility trick — all worked
with no CNA-side gaps for the sample's own code. While converting `terrain.fbx`,
found and fixed a real bug in `tools/fbx_ascii2model.py`: it never applied a mesh's
baked `PreRotation`/`LclRotation`/`LclScaling`/`LclTranslation` node transform, only
its raw vertex/normal data. `terrain.fbx`'s `Plane01` mesh has a `-90,0,0`
`PreRotation` (a standard 3ds-Max Z-up → FBX-declared-Y-up correction); without
applying it, the terrain's height ended up in Z instead of Y and the whole mesh sat
outside the camera's view volume — first screenshot was solid CornflowerBlue with
nothing visible at all. Fixed the converter generically (any mesh's node transform,
not special-cased to this asset) and confirmed live it does not change `tank.fbx`'s
already-shipped conversion output (all its `PreRotation`s are `0,0,0`; regenerated
into a scratch dir and diffed byte-identical against the checked-in
`tank.model.json`/`.bin` files — see DEFERRED.md item #6's addendum for the note
that `tank.fbx`'s *sub-meshes* do have non-zero `Lcl Translation`, relevant only
once a future sample needs independently-posed tank parts).

After the fix, LensFlare's terrain renders with the same thin-diagonal-line
near-plane-clipping artifact already known from `CameraShake`/`CustomModelClass`'s
tank rendering — confirming that bug's cause is shared across two independently-
converted FBX assets, not specific to the tank model. Also found a **new** CNA
rendering gap while verifying: the EasyGL backend never applies
`BlendState.ColorWriteChannels` (`glColorMask` is never called anywhere in
`EasyGLGraphicsBackend` — confirmed via direct grep), so `LensFlareComponent`'s
occlusion-query trick (drawing an intentionally-invisible test polygon via
`ColorWriteChannels.None`) instead renders a fully visible white square on screen.
Filed as DEFERRED.md item #22 (not started — not fixed this session, out of scope
for a porting task). Additionally observed that the glow/flare sprites never
appeared during 5+ seconds of live verification; not root-caused, flagged in
`samples/LensFlare/missing.md` rather than assumed benign.

F1 help overlay verified via this repo's established temporary-debug-auto-trigger
pattern (removed before commit) — `xdotool` reached the window's focus
(`getactivewindow` matched) but a sent `F1` keypress had no observable effect,
consistent with this repo's known `xdotool` reliability caveat, not a code bug.

**Also discovered, unrelated to LensFlare:** a full aggregate build
(`cmake --build cmake-build-debug -j$(nproc)`, no `--target`) now fails with 8
errors in `samples/SafeArea/src/SafeAreaOverlay.hpp` — `Viewport` no longer exposes
direct `.x`/`.y` members upstream in `cna` (same class of drift that previously
broke `InputReporter`). Not fixed this session (out of scope for the LensFlare
task); flagged as a new item in section 8 for whoever picks this repo up next.
`LensFlare_cna_samples` itself was independently confirmed to build clean via its
own `--target`, both before and after this discovery.

Commits this session: not yet committed as of this NEXT.md update — see git status.

**Newest session (2026-07-06, follow-up):** `cna`'s `feature/net` fixed two of the
three DEFERRED.md gaps `ClientServerSample` (#091) worked around — #19
(`GamerServicesDispatcher` hang, commit `08171ac`) and #20 (`NetworkGamer.IsHost`/`Id`
stubs, commit `81f10b5`). Removed both workarounds from `ClientServerSample`: a real
`GamerServicesComponent` is now constructed (matching the C# original exactly), and
the local `bool isHost_` tracking member is gone in favor of
`networkSession_->getIsHostProperty()`/`gamer->getIsHostProperty()` directly. Built
against `cna`'s `feature/net` tip (temporarily checked out in the `../cna`
build-dependency checkout, not merged into `cna`'s own `develop` — that's a separate
decision for whoever manages that branch; **a future session should either merge
`feature/net` → `develop` in `cna`, or re-check-out `feature/net` locally again, before
relying on these fixes being present**). Live-verified with real `xdotool` keypresses
(unlike the prior session, which needed a debug auto-trigger): session creation no
longer hangs, tank spawns labeled `"Stub Gamer (server)"` (the real
`GamerServicesDispatcher`-populated identity, not the old manually-synthesized
`"Player"`), movement works. A genuine two-instance host+client test hit a separate,
pre-existing `ENetDiscoveryService` two-process discovery limitation on this
container (not a regression) — see `samples/ClientServerSample/missing.md`.
Item #21 (`GamerJoined` queued, not synchronous) was investigated in `cna` in depth
and found to require either a `sharp-runtime` change (needs the user's direct
sign-off) or accepting the sample's existing extra `Update()`-after-subscribing call
as the permanent, correct pattern. Updated `DEFERRED.md` items #19–21 and
the summary table. Commit `3197b06`, pushed to `develop`.

**Same-day follow-up: the user approved the `sharp-runtime` change, and it landed.**
`sharp-runtime`'s `EventHandler<T>` gained a generic, opt-in `SetReplayHook()`
(`develop` commit `69661c2`); `cna`'s `NetworkSession` constructor now uses it so
`GamerJoined` replays immediately on subscription, matching real XNA (`feature/net`
commit `ab05395`). Removed `ClientServerSample`'s last remaining workaround too — the
`networkSession_->Update();` call right after `HookSessionEvents()` — confirmed live
it's genuinely unnecessary now. `xdotool` was unreliable on this shared desktop again
this round (tried real keypresses, a fully isolated `Xvfb :77` display with no window
manager, both failed identically — confirmed environmental, not a regression); fell
back to this repo's own established debug-auto-trigger pattern (temporary, removed
before commit) to verify live: session creation → immediate `GamerJoined` → tank
spawn → render, no crash, no manual `Update()` needed. `ClientServerSample` now has
**zero** of its original three DEFERRED.md workarounds. Updated `DEFERRED.md` item
#21 (now ✅ resolved) and `missing.md`. Commit `ef1e930`, pushed to `develop`.

**Not done this session:** re-attempting NetworkPrediction (#100)/PeerToPeer (#103)
— both share the same three gaps and are now easier to port than ever (all 3
workarounds gone), but porting two new samples from scratch is a separate, larger
task than this session's cleanup scope; a natural next step if the user wants it.

Most recent full porting session (2026-07-06), in order:
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

Commits from the newest follow-up session (see the top entry above): `3197b06`,
`ef1e930`. Both pushed to `develop`.

---

## 4. Current blocker / main problem

**A full aggregate build (`cmake --build cmake-build-debug -j$(nproc)`, no
`--target`) now fails** — newly discovered 2026-07-09, unrelated to any of this
session's own changes:

- **Exact symptom:** 8 compile errors in
  `samples/SafeArea/src/SafeAreaOverlay.hpp` (lines 47/48/50/51/53/56/58):
  `'class Microsoft::Xna::Framework::Graphics::Viewport' has no member named 'x'`
  (and `'y'`).
- **Failing command:** `cmake --build cmake-build-debug -j$(nproc)` (aggregate;
  any single sample's own `--target` build is unaffected unless it also touches
  `Viewport.x`/`.y` directly — not audited beyond `SafeArea`, the only sample the
  aggregate build reached before `ninja` stopped on the first failure).
- **Root cause:** `Viewport` no longer exposes direct `.x`/`.y` members upstream in
  `cna` (must now use `getXProperty()`/`getYProperty()`) — same class of drift that
  previously broke `InputReporter`'s direct `GamePadCapabilities` field access
  (section 3's history).
- **Fix shape:** mechanical, ~7-line change in one file — switch each
  `viewport.x`/`viewport.y` to `viewport.getXProperty()`/`viewport.getYProperty()`.
  Not done this session (out of scope for the task that found it — porting
  LensFlare); see section 8 for a dedicated task.

The previously-tracked rendering bug (near-plane clipping) is still open and is now
confirmed on a **third** independent asset, and with a **second distinct visible
symptom**:

- **Exact symptom:** a `Model` drawn through a perspective camera renders as a
  thin diagonal line/dashes instead of a recognizable 3D shape — at "moderate"
  camera distance (~1000 units). Originally found on `tank.model.json`
  (CameraShake/CustomModelClass); confirmed on LensFlare's `terrain.model.json`
  too. **At longer camera distance (~3500 units), the same underlying bug
  instead produces full invisibility, not a thin line** — confirmed 2026-07-09
  while porting Graphics3D (#046): its spaceship (own camera distance ~3523)
  renders nothing at all; swapping the already-proven `tank.model.json` into
  Graphics3D's own drawing code at that same ~3523 distance also rendered
  nothing, and swapping it in again at `CustomModelClass`'s own ~1059 distance
  reproduced the familiar thin line — isolating the distance, not the asset or
  the drawing code, as what determines which symptom appears.
- **Failing command (to reproduce):**
  ```
  cd cmake-build-debug/samples/CameraShake
  SDL_VIDEODRIVER=x11 ./CameraShake_cna_samples
  ```
  (or `CustomModelClass_cna_samples`, `LensFlare_cna_samples` — thin line; or
  `Graphics3D_cna_samples` — fully invisible, same bug family, longer camera
  distance.)
- **No failing automated test** — there is no test suite; this was found by
  screenshot comparison.
- **Affected files/modules:** almost certainly
  `cna/src/CNA/Internal/Backends/EasyGL/EasyGLGraphicsBackend.cpp` (clipping/
  projection path) — not yet confirmed by direct inspection.
- **Suspected cause:** near-plane (`w<0`) vertex clipping in the EasyGL backend
  does not match DirectX/real-XNA behavior, so triangles crossing the near plane
  degenerate instead of being clipped correctly. The distance-dependent
  thin-line-vs-invisible split found this session suggests whatever's wrong is
  sensitive to the actual clip-space `w`/depth values involved, not purely a
  binary "does this triangle cross the near plane" check — worth keeping in
  mind once someone actually opens `EasyGLGraphicsBackend.cpp` to fix this.
- **What has been tried:** confirmed (2026-07-06) via a side-by-side screenshot
  that CameraShake and CustomModelClass show the *identical* artifact in the
  *identical* screen position; confirmed again (2026-07-09) that LensFlare's
  independently-converted terrain asset shows the same artifact too; confirmed
  again (2026-07-09) via direct asset-swap isolation testing that Graphics3D's
  full invisibility is the same bug at a different camera distance, not a
  separate defect. Not yet root-caused inside `cna` itself; no fix attempted.
- **Why it matters now:** 6 more samples (PickingSample, TrianglePicking,
  HeightmapCollision, InverseKinematics, ChaseCamera, MarbleMaze — LensFlare and
  Graphics3D are now both ported, see section 8) are otherwise unblocked and
  portable, but **should not be assumed to render correctly** just because they
  build — each needs its own screenshot check for this same artifact once
  ported (and, per the above, "renders nothing" is now just as suspect as "shows
  a thin line" — don't assume a blank frame means something else is wrong
  without checking camera distance against this bug first).

**Other rendering/framework gaps found this session (not blocking, tracked
separately):**
- EasyGL backend never applies `BlendState.ColorWriteChannels` — DEFERRED.md
  item #22 (found via LensFlare).
- `Game::DoInitialize()` wires `Components_.ComponentAdded` after calling the
  user's `Initialize()` override, breaking the common "add components from
  `Initialize()`" pattern real XNA/FNA supports — DEFERRED.md item #23 (found
  via Graphics3D; worked around at the sample level).
- `GraphicsDevice::Clear(Color)`'s single-argument overload never clears the
  depth buffer (every 3D sample in this repo uses it) — DEFERRED.md item #24
  (found via Graphics3D; confirmed not the cause of its invisible-model bug
  above, but a real, separate, latent gap).

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
  `Model`-based geometry (confirmed: `tank.model.json` at CameraShake's and
  CustomModelClass's ~1059-unit camera distance, `terrain.model.json` at
  LensFlare's) as a degenerate thin line instead of the model — **and, confirmed
  2026-07-09 via Graphics3D, the same bug renders geometry as fully invisible at
  longer (~3523-unit) camera distances**, isolated by direct asset-swap testing
  to be the camera distance, not the asset or drawing code. See section 4 for
  full detail. Confirmed on three independently-converted assets across two
  distinct visible symptoms; likely affects other models/samples too — don't
  assume "renders nothing" means something else is broken without checking this
  first.
- **CONFIRMED BUG, unfixed** — the EasyGL backend never applies
  `BlendState.ColorWriteChannels` (no `glColorMask` call anywhere in
  `EasyGLGraphicsBackend.cpp`, confirmed via direct grep). Found via LensFlare's
  occlusion-query trick (a `ColorWriteChannels.None` blend state meant to keep its
  test polygon invisible), which instead renders a solid white square on screen.
  DEFERRED.md item #22 (new, not started). See `samples/LensFlare/missing.md`.
- **CONFIRMED BUG, worked around (Graphics3D, #046)** — `Game::DoInitialize()`
  wires up `Components_.ComponentAdded`/`ComponentRemoved` *after* calling the
  user's `Initialize()` override, unlike real XNA/FNA (which subscribes in the
  `Game` constructor, before `Initialize()` can run). A component added to
  `Components` from within `Initialize()` — a pattern real XNA supports and this
  sample's C# original uses — never gets its own `Initialize()`/`LoadContent()`
  called, since the event that would trigger it isn't subscribed yet; segfaults
  on first `Draw()` if that component's `Draw()` depends on `LoadContent()`
  having run. Worked around by calling `component->Initialize()` explicitly
  right after `Add()`. DEFERRED.md item #23 (new, not started). See
  `samples/Graphics3D/missing.md`.
- **CONFIRMED BUG, unfixed (latent)** — `GraphicsDevice::Clear(Color)`'s
  single-argument overload never clears the depth buffer (confirmed via direct
  source read); real XNA's same-signature overload clears color+depth+stencil
  together. Every 3D sample in this repo uses this overload. Confirmed (via
  Graphics3D's investigation) this is *not* the cause of the near-plane-clipping
  bug above, but is a real, separate, currently-latent gap. DEFERRED.md item #24
  (new, not started).
- **NEW BUILD BREAKAGE, unfixed** — `samples/SafeArea/src/SafeAreaOverlay.hpp`
  accesses `Viewport.x`/`.y` directly; `cna`'s `Viewport` no longer exposes those as
  public members (property-getter-only now). Breaks both `SafeArea`'s own
  `--target` build and, as a consequence, the full aggregate build (`ninja` stops
  at the first failure). Every other sample's own `--target` build is unaffected.
  See section 4 for the exact errors and section 8 for a dedicated fix task.
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

**Recently completed (2026-07-09):** LensFlare (#041) and Graphics3D (#046),
both screenshot-verified — see section 3 for the full account of each,
including the new DEFERRED.md items (#22–#24) both surfaced.

1. **Fix `SafeArea`'s `Viewport.x`/`.y` build breakage — blocks the full
   aggregate build.**
   - Goal: `samples/SafeArea/src/SafeAreaOverlay.hpp` (lines 47/48/50/51/53/56/58)
     accesses `viewport.x`/`viewport.y` directly; `cna`'s `Viewport` no longer
     exposes those as public members. Switch each to
     `viewport.getXProperty()`/`viewport.getYProperty()` (see `CLAUDE.md`'s
     property-access table).
   - Files: `samples/SafeArea/src/SafeAreaOverlay.hpp`.
   - Verify: `cmake --build cmake-build-debug --target SafeArea_cna_samples`,
     then `cmake --build cmake-build-debug -j$(nproc)` (full aggregate) to
     confirm no other sample hits the same `Viewport.x`/`.y` pattern.

2. **Investigate the EasyGL near-plane clipping bug itself (section 4).**
   - Goal: clip `w<0` vertices correctly so tank/terrain/spaceship models render
     fully instead of degenerating to a thin line (moderate camera distance) or
     disappearing entirely (longer camera distance — confirmed 2026-07-09 via
     Graphics3D that this is the *same* bug, not a separate one). Now confirmed
     on three independent assets (`tank.model.json`, `terrain.model.json`,
     `spaceship`-via-`.obj`), raising confidence this is a real shared
     clipping/projection bug, not an asset artifact.
   - Files: likely `cna/src/CNA/Internal/Backends/EasyGL/
     EasyGLGraphicsBackend.cpp` (clipping/projection path) — exact location not
     yet confirmed.
   - Verify: run `CameraShake_cna_samples`, `CustomModelClass_cna_samples`,
     `LensFlare_cna_samples`, and `Graphics3D_cna_samples` under
     `SDL_VIDEODRIVER=x11`; confirm the tank/ground/terrain/spaceship are fully
     visible in a screenshot at both camera-distance regimes, not a thin line or
     nothing.

3. **Fix the EasyGL `BlendState.ColorWriteChannels` gap (DEFERRED.md item #22).**
   - Goal: `EasyGLGraphicsBackend` never calls `glColorMask` (or equivalent), so
     `ColorWriteChannels.None`/partial-channel blend states are silently ignored
     — every draw writes all 4 color channels regardless of the active
     `BlendState`. Found via LensFlare's occlusion-query trick, which renders a
     visible white square instead of staying invisible.
   - Files: wherever `EasyGLGraphicsBackend` applies `BlendState` to GL state
     (alongside its existing blend-func/equation setup) — exact location not yet
     confirmed.
   - Verify: run `LensFlare_cna_samples` under `SDL_VIDEODRIVER=x11`; confirm the
     occlusion-query polygon is no longer visible as a white square. Consider
     also re-checking whether this fixes the "glow/flare sprites never appear"
     observation in `samples/LensFlare/missing.md` — not established either way
     yet.

4. **Fix `Game::DoInitialize()`'s `ComponentAdded` subscription timing
   (DEFERRED.md item #23).**
   - Goal: move the `Components_.ComponentAdded +=`/`ComponentRemoved +=`
     subscription in `Game::DoInitialize()` to before the call to `Initialize()`
     (or into `Game`'s constructor, matching FNA/XNA exactly), so components
     added to `Components` from within a user `Initialize()` override get their
     own `Initialize()`/`LoadContent()` called automatically, like real XNA.
   - Files: `cna/src/Microsoft/Xna/Framework/Game.cpp` (`DoInitialize()`).
   - Verify: remove Graphics3D's `AddComponent()` workaround (its explicit
     `component->Initialize()` calls) and confirm its 4 `Checkbox`es still work
     correctly — this is the regression test.

5. **Fix `GraphicsDevice::Clear(Color)` to also clear the depth buffer
   (DEFERRED.md item #24).**
   - Goal: make the single-argument `Clear(Color)` overload match real XNA —
     clear color + depth + stencil together, not just color.
   - Files: `cna/src/Microsoft/Xna/Framework/Graphics/GraphicsDevice.cpp`.
   - Verify: re-run the near-plane-clipping repro samples (task 2) after this
     fix lands, in case stale depth-buffer contents were contributing to any of
     their symptoms (not established either way this session — the two bugs
     were tested independently and this one wasn't confirmed as a contributing
     cause, but wasn't fully ruled out as a compounding factor either).

6. **Port one of the 6 remaining unblocked lighting samples** (PickingSample,
   TrianglePicking, HeightmapCollision, InverseKinematics, ChaseCamera,
   MarbleMaze).
   - Goal: same pattern as LensFlare/Graphics3D — port using stock
     `Model`/`BasicEffect`, screenshot-verify, expect (per task 2) either the
     thin-line or fully-invisible near-plane-clipping symptom depending on
     camera distance; that alone is not a reason to suspect a new bug.
   - Notes from a quick asset survey this session (not yet re-verified per
     sample, just source/asset inspection):
     - **HeightmapCollision** — terrain is generated at content-build time from
       a `terrain.bmp` heightmap via a custom content-pipeline processor (no
       plain `.fbx`/`.x` for it); would need either a small Python heightmap→
       `.model.json` generator or a runtime procedural-mesh approach (this repo
       already has a terrain-generation precedent in `GeneratedGeometry`).
     - **ChaseCamera**, **InverseKinematics** — each needs one `.x`-format model
       (`Ground.x`, `cylinder.x` respectively) that neither `tools/
       fbx_ascii2model.py` nor `tools/obj2model.py` reads directly; would need
       an `assimp`/Blender `.x`→`.obj` conversion step first (both tools do
       support `.x` import, unlike the old-binary-FBX case Graphics3D hit).
     - **PickingSample**, **TrianglePicking** — multiple `.fbx` models each
       (table/sphere/cylinder/p2wedge/cats), all plain ASCII or otherwise
       normally-convertible FBX as far as a quick `file`-command check showed;
       more assets to convert than LensFlare/Graphics3D but no known format
       blocker.
     - **MarbleMaze** — much larger source tree (140 files); only a specific
       subdirectory (`Source/EX2_Polishing/End/`) is likely the actual port
       target per its `missing.md` — recommend doing this one last.
   - Files: new `samples/<Name>/src/`; read that sample's existing
     `missing.md` first.
   - Verify: `cmake --build cmake-build-debug --target <Name>_cna_samples`, run
     under `SDL_VIDEODRIVER=x11`, screenshot.

7. **Port NetworkPrediction (#100) or PeerToPeer (#103).**
   - Goal: all three of ClientServerSample's original workarounds are gone now
     (DEFERRED.md #19/#20/#21 all fixed upstream in `cna`/`sharp-runtime` — see
     section 3's newest entries): a real `GamerServicesComponent` can be constructed
     normally, `networkSession_->getIsHostProperty()`/`gamer->getIsHostProperty()`
     can be used directly instead of a local tracking bool, and `GamerJoined`
     replays immediately on subscription (no manual `Update()` call needed right
     after hooking events). These two new samples should need **zero**
     networking-related workarounds — a good test of whether that claim actually
     holds for their own `Update()` loop shape.
   - **Before starting:** make sure `../cna`'s checkout actually has these fixes —
     either `git -C ../cna checkout feature/net` (what this session used, not merged
     to `develop` yet) or check whether someone has since merged `feature/net` →
     `develop` there. Also confirm `../sharp-runtime` (relative to `../cna`) is on a
     `develop` that includes commit `69661c2` (`EventHandler::SetReplayHook`).
   - Files: new `samples/NetworkPrediction/src/` or `samples/PeerToPeer/src/`;
     read `samples/ClientServerSample/missing.md` and DEFERRED.md items #19–21
     first.
   - Verify: `cmake --build cmake-build-debug --target NetworkPrediction_cna_samples`
     (or `PeerToPeer_cna_samples`); screenshot the menu + a triggered session
     the way ClientServerSample was verified.

8. **Fix the Vulkan multiple-SpriteBatch-per-frame bug.**
   - Goal: a second `Begin()/End()` in the same frame must not discard the
     first.
   - Files: `cna/src/.../Vulkan/VulkanGraphicsBackend.cpp`.
   - Verify: run GameStateManagement or CatapultWars on the Vulkan backend;
     confirm all layers draw.

9. **Decide the scope for AccelerometerSample (#084)/TiltPerspective (#107).**
   - Goal: get an explicit go/no-go from the user on inventing a keyboard-tilt
     fallback from scratch (neither original has any non-phone code to reuse)
     before spending the effort.
   - Files (if approved): new `samples/AccelerometerSample/src/`,
     `samples/TiltPerspective/src/`.
   - Verify: `cmake --build cmake-build-debug --target AccelerometerSample_cna_samples`.

10. **Decide whether to port any of the 5 now-reopened Avatar samples** (#085,
   #086, #087, #094, #101) onto `cna`'s new `AvatarRenderer::
   EnableRealRenderingEXT` substitute-body path.
   - Goal: get an explicit go/no-go from the user before spending effort — this
     is a product decision (substitute art, not the real Xbox Avatar look), not
     a technical blocker.
   - Files: none yet.
   - Verify: N/A until a decision is made.

11. **(User-owned, tracked for visibility) Add per-mesh `ModelBone` support to
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
  yet. This does **not** apply to the 6 remaining lit-`BasicEffect`-only samples
  (LensFlare and Graphics3D are now ported — section 8, task 6) — those need no
  shader work.
- **Do not start a skeletal-animation sample** (SkinningSample,
  CustomModelAnimation, SkinnedModelExtensions, CPUSkinning) without
  `AnimationClip`/`Keyframe`/`AnimationPlayer` existing in `cna` (item #13).
- **Do not invent a keyboard-tilt input scheme for AccelerometerSample/
  TiltPerspective** without an explicit scope decision first (section 8, task 9).
- **Do not start porting any of the 5 reopened Avatar samples** without an
  explicit scope decision first (section 8, task 10).
- **Do not assume a newly-unblocked lighting sample renders correctly just
  because it builds** — screenshot it and check for the near-plane clipping
  artifact (section 4) first. Remember it can now manifest as either a thin
  line *or* full invisibility depending on camera distance (confirmed via
  Graphics3D) — don't assume "renders nothing" must be a different, new bug
  without checking this first.
- **Do not add a shared `samples/common/` library**, even where two samples'
  code looks structurally similar.
- **Do not edit `cna`/`sharp-runtime` source** without confirming scope with the
  user first — they are independently active sibling repos.
- **Do not hammer `xdotool` input** without re-verifying window focus
  immediately beforehand, and do not conclude "no visible effect" means a code
  bug without first checking the sample's own state.
- **No broad refactors or unrelated cleanup** while the near-plane-clipping/
  `ColorWriteChannels`/`ComponentAdded`-timing/`Clear(Color)`-depth/
  `SafeArea`-build/Vulkan bugs (section 8, tasks 1–5/8) are open.
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
