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

**Current phase:** 55 samples are fully ported and wired into the root
`CMakeLists.txt`. 1 more is confirmed unblocked (no remaining CNA gap) and ready
to port (MarbleMaze — see section 8 task 6; ChaseCamera and InverseKinematics were
the other two candidates from the original list and are now both ported, see
section 3). 28 placeholder directories exist for
samples still genuinely blocked on real CNA engine work (custom shaders, skeletal
animation, one content-pipeline gap). 67 catalogued directories are permanently out
of scope and listed in `ignored.md` (not XNA 4.0, not a runnable `Game`, redundant
duplicates, or tied to a platform CNA won't target). See `PLAN.md`'s Sample Count
Summary table for exact per-category counts.

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
- **ChaseCamera (#058) ported** (2026-07-10) — a spring-physics chase camera
  (`ChaseCamera.hpp`, pure `Vector3`/`Matrix` math ported directly, no CNA gaps)
  following a ship (`Ship.hpp`, simple flight physics + mouse/keyboard/gamepad
  input) flying over a large checkered ground plane. Builds 0 warnings, runs 8+
  seconds with no crash across three separate runs. **A third and fourth
  independent confirmation of DEFERRED.md item #26's `ModelTypeReader`
  vertex-corruption bug**, per this task's own brief to test empirically rather
  than assume: a temporary test build using plain `Content.Load<Model>("Ship")`/
  `Content.Load<Model>("Ground")` (converted via `tools/fbx_ascii2model.py` and
  `assimp export` + `tools/obj2model.py` respectively — both ordinary stride-32
  `.model.json` files, no load-time error) rendered **nothing at all** — a solid
  CornflowerBlue screen with only 2D HUD text visible, confirmed via two
  screenshots 3 seconds apart, no crash — at this sample's own ~4031-unit initial
  camera distance (even farther than Graphics3D's ~3523-unit spaceship, which
  showed the same "fully invisible" symptom). Worked around with a new
  `samples/ChaseCamera/src/RawModel.hpp` (NOXNA), the same bypass shape as
  InverseKinematics' `CylinderModel.hpp`/HeightmapCollision's `Terrain.hpp`,
  generalized to also bind a real `Texture2D` directly to the `BasicEffect` (the
  same side benefit `Terrain.hpp` already established) — confirmed live via
  screenshot: both the ship (`ShipDiffuse.png`, 32458 vertices — two orders of
  magnitude larger than InverseKinematics' 418-vertex cylinder) and the ground
  (`Checker.png`, only 6 vertices — the smallest mesh yet tested through this
  bug) render correctly, fully textured and shaded, at both size extremes. This
  independently reinforces item #26's hypothesis a third and fourth time, on two
  more assets converted through two different pipelines (FBX and `assimp`
  `.x`→`.obj`), uncorrelated with mesh size/complexity. Separately found (not a
  #26 symptom): `Ground.x`'s `assimp export` conversion re-emits its two
  triangles wound the opposite way from CNA's default
  `RasterizerState::CullCounterClockwise`, so the ground was fully back-face
  culled even after the `RawModel.hpp` fix — isolated live by temporarily
  forcing `RasterizerState::CullNone` around only the ground's draw call, which
  alone made it appear; kept as a permanent, documented per-asset adjustment
  (mirroring the same winding accommodation `HeightmapCollision`'s/
  `GeneratedGeometry`'s own `Terrain.hpp` already needed for their runtime-built
  meshes, just the first time seen on an `assimp`-round-tripped `.x` asset
  specifically). F1 help overlay and ship-movement/camera-spring-physics both
  verified live via this repo's established temporary debug-auto-trigger pattern
  (removed before commit) — `xdotool getactivewindow` showed a different real
  user window had focus throughout, so no synthetic keypresses were sent, per
  this repo's own shared-desktop `xdotool` caveat. See
  `samples/ChaseCamera/missing.md` for the complete account.
- **InverseKinematics (#057) ported** (2026-07-10) — the Cyclic Coordinate Descent
  (CCD) inverse-kinematics algorithm, demonstrated via a 20-link chain of cylinder
  models reaching for a billboarded "cat" sprite (plus a faithfully-ported, but
  legitimately inert, Xbox LIVE avatar IK demo — see below). Builds 0 warnings,
  runs 9+ seconds with no crash. **Found and worked around a major, previously
  undiagnosed CNA bug**, likely the true root cause of the long-tracked
  "near-plane-clipping" bug family (section 4): `ModelTypeReader::Read()`
  (`ContentManager.cpp`) picks a typed `VertexBuffer::SetData` overload by
  comparing a `.model.json`'s declared `"vertexStride"` (always one of XNA's
  clean, unpadded sizes — 16/20/24/32) against `sizeof()` of CNA's own vertex
  structs — but every one of those structs now inherits from the polymorphic
  `IVertexType`, adding an 8-byte vtable pointer that inflates their real sizes to
  40/32/56/40 respectively (confirmed live via a standalone `sizeof()` probe).
  `"vertexStride": 32` — the value every `Content.Load<Model>`-based sample in
  this repo uses — *accidentally* equals `sizeof(VertexPositionTexture)` (also
  32, by coincidence), so the reader always dispatches to the *wrong* overload
  and reinterprets the raw (vtable-free) file bytes as vtable-shifted
  `VertexPositionTexture` objects, reading position/texcoord data from the wrong
  byte offsets — silently corrupting every stride-32 model's vertex data
  repo-wide, not just this sample's. Found via systematic isolation: a
  straightforward `cylinder.model.json` port built and loaded correctly (1 mesh,
  418 vertices, verified via debug instrumentation) but never rendered at any
  camera distance/scale/cull-mode; a hand-built triangle and even PickingSample's
  own already-shipped `Cylinder.model.json` both failed the same way through this
  sample's code, while HeightmapCollision's `sphere.model.json` (3252 vertices)
  rendered correctly at every scale tested — isolating the reader, not any one
  asset, as the cause. Worked around with a new `CylinderModel.hpp` (NOXNA) that
  reads the already-converted `cylinder_verts.bin`/`cylinder_idx.bin` directly and
  constructs real C++ `VertexPositionNormalTexture` objects field-by-field (not a
  `reinterpret_cast` on a raw blob), then uploads via the same typed `SetData`
  overload the reader was trying to reach — confirmed live this renders correctly
  (a lit, visibly-shaded, correctly-curling 20-link chain). Filed as new
  DEFERRED.md item #26, with a strong (but not 100%-confirmed-for-other-samples)
  suspicion this explains the thin-line/invisibility symptom for every other
  affected sample too — see section 4 and item #26 for the full reasoning and a
  recommended next step (try the same bypass on CameraShake's `tank.model.json`
  before assuming the bug is really in clip-space math). The avatar half of the
  sample (`AvatarRenderer`) is ported faithfully but renders nothing — confirmed
  by direct source read that `AvatarRenderer::State` is permanently
  `Unavailable` off a signed-in Xbox LIVE/Games-for-Windows-Live session, exactly
  what the C# original's own `UpdateAvatarIK()`/`DrawAvatar()` already guard
  against — not a CNA gap. See `samples/InverseKinematics/missing.md` for the
  complete account.
- **HeightmapCollision (#049) ported** (2026-07-10) — a rolling ball on a
  procedurally-generated terrain, with the camera and ball both queried against a
  `HeightMapInfo` height-lookup class. Builds 0 warnings, runs 7+ seconds with no
  crash across two separate runs; screenshots show a fully **textured and shaded**
  terrain (unlike every other `Content.Load<Model>`-based sample so far) with the
  ball sitting correctly on its surface. The interesting engineering question was
  the terrain: XNA's original generates it at content-**build** time from
  `terrain.bmp` via a custom `TerrainProcessor`/`HeightMapInfoContent`
  `ContentProcessor` pair, attaching the collision height data to the built
  `Model`'s `Tag`. CNA has neither a `Model.Tag` equivalent nor custom-
  `ContentProcessor` extensibility (item #18), so — after reading this repo's own
  `GeneratedGeometry` sample first and confirming it already ships a *structurally
  identical* `TerrainProcessor` in its own C# original — this port builds the
  terrain mesh **and** the `HeightMapInfo` collision data together at runtime
  (`Terrain.hpp`, NOXNA), the same adaptation `GeneratedGeometry` already
  established, not a new pattern. A second, independent reason favored this over
  `Content.Load<Model>`: `terrain.bmp` is 257×257 = 66049 vertices, over the 65535
  limit of a 16-bit index buffer, and a direct source read confirmed CNA's
  `.model.json` `ModelTypeReader` hardcodes 16-bit indices with no way to request
  32-bit — a genuine, narrower nuance of item #6 not previously documented (added
  as a new addendum this session), even though `IndexBuffer` itself fully supports
  `IndexElementSize::ThirtyTwoBits` end-to-end. Building the terrain directly with
  a real 32-bit `IndexBuffer` sidestepped both gaps at once. A pleasant side
  effect: because the terrain never goes through `.model.json` (which has no
  per-mesh texture field — item #6's PickingSample/TrianglePicking addendum), its
  hand-built `BasicEffect` + real bound `Texture2D` renders **correctly textured
  and shaded** — the sphere, which *is* loaded via `Content.Load<Model>("sphere")`,
  still hits the known flat-white gap as expected. This sample's camera sits only
  ~155 units from the ball (much closer than the ~1000+ unit distances that
  trigger the tracked near-plane-clipping bug elsewhere in this repo) — confirmed
  via screenshot that neither the ball nor the terrain shows that artifact here;
  not claiming the bug is fixed, just that this sample's own geometry doesn't
  trigger it. This sample also adds no `GameComponent`s at all, so item #23 simply
  doesn't apply. See `samples/HeightmapCollision/missing.md` for the full account.
- **TrianglePicking (#048) ported** (2026-07-09) — close sibling of PickingSample
  (#047, byte-identical FBX assets confirmed via `cmp`), replacing its per-object
  `BoundingSphere`-only test with real per-triangle ray intersection
  (Moller-Trumbore). CNA has no `Model.Tag`/custom-`ContentProcessor` equivalent
  (item #18) *and*, confirmed this session, no `VertexBuffer`/
  `IndexBuffer.GetData()` runtime readback either (new DEFERRED.md item #25) — the
  C# original's own per-triangle vertex data (normally attached to `Model.Tag` at
  content-build time by a custom `TrianglePickingProcessor`) is instead produced
  by a new `--picking` option added to `tools/fbx_ascii2model.py`, which emits a
  flat binary sidecar of triangle-expanded vertex positions per model, read back
  directly by the C++ port at `LoadContent()` time. Builds 0 warnings, runs 7+
  seconds with no crash. Confirmed the already-tracked near-plane-clipping
  thin-line artifact on the `Sphere` model (consistent every frame, since this
  sample's camera doesn't auto-rotate unlike PickingSample's port) and the
  already-tracked flat-white-saturation finding (DEFERRED.md item #6 addendum) —
  both immediately recognized as known, not re-diagnosed. Also clarified
  DEFERRED.md item #23 (`Game::DoInitialize()`'s `ComponentAdded` timing gap):
  confirmed this sample's C# original adds its `Cursor` component from the
  *constructor*, not `Initialize()` (unlike PickingSample/Graphics3D), and by
  reading `Game.cpp` directly, confirmed this genuinely does **not** need the
  `AddComponent()` workaround — CNA's base `Game::Initialize()` has its own
  unconditional per-component-initialize pass that already catches
  constructor-time adds regardless of `ComponentAdded` subscription timing.
  Unexpectedly got a live (non-debug-forced) confirmation that real per-triangle
  picking works correctly end-to-end: an earlier `xdotool mousemove --window`
  call (issued while checking this repo's known `xdotool` focus caveat) moved the
  real OS pointer over the window even without confirmed focus, and a later clean
  screenshot shows correct bounding-sphere-list text and a correct per-triangle
  picked-model name label. See `samples/TrianglePicking/missing.md` for the full
  account.
- **PickingSample (#047) ported** (2026-07-09) — `DrawableGameComponent`
  (`Cursor`), stock `Model`/`BasicEffect`, `Viewport::Project`/`Unproject`,
  `Ray`/`BoundingSphere::Intersects` for mouse-ray picking against 5 FBX models
  (table + 4 picked objects). Builds 0 warnings, runs 5+ seconds with no crash.
  All 5 source FBX files were plain ASCII, converted directly with
  `tools/fbx_ascii2model.py` (no binary-FBX workaround needed this time). Hit
  and worked around the same `Game::DoInitialize()` component-lifecycle gap
  Graphics3D found (DEFERRED.md item #23 — `Cursor` is created and added to
  `Components` from inside `Initialize()`, matching the C# original). Also hit
  and worked around a related, previously-only-implicitly-documented gap:
  `ModelMesh::ParentBone` is always `nullptr` for `.model.json`-loaded models
  (DEFERRED.md item #6's existing "multi-bone" note already covered the root
  cause) — added a `BoneIndexOf()` fallback-to-0 helper, the same guard CNA's
  own `Model::Draw()` already uses internally. Found that every model renders
  as a flat, fully-saturated white shape with zero shading gradient (confirmed
  angle-independent via multiple screenshots) — root-caused to the combination
  of CNA's `.model.json` format having no per-mesh texture field (an existing
  gap, first flagged by LensFlare's `ground.png`) and XNA's bright default
  3-point lighting rig clamping to white on an untextured, default-white
  material; added an addendum to DEFERRED.md item #6 documenting this more
  severe consequence (not a new item — same root cause). Also separately
  observed the already-tracked near-plane-clipping thin-line artifact once, at
  one camera angle, confirming it's independent of the flat-white finding. See
  `samples/PickingSample/missing.md` for the full account.
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

**Newest session (2026-07-10, second follow-up):** Ported **ChaseCamera (#058)**,
section 8 task 6's next candidate after InverseKinematics — a spring-physics chase
camera (`ChaseCamera.cs`, pure `Vector3`/`Matrix` math with no CNA API surface
beyond what every other 3D sample already uses) following a ship (`Ship.cs`,
simple flight physics driven by keyboard/gamepad/mouse) flying over a large
checkered ground plane. `MarbleMaze` is now the only sample left from the
original 3-candidate list (see section 8).

This session's task brief specifically flagged DEFERRED.md item #26 (the
`ModelTypeReader` vertex-corruption bug found while porting InverseKinematics)
and asked for an empirical test, not an assumption, before treating this
sample's own models as affected. That test was done first: a temporary build
using plain `Content.Load<Model>("Ship")`/`Content.Load<Model>("Ground")` (`Ship.fbx`
converted directly via `tools/fbx_ascii2model.py`; `Ground.x` converted via
`assimp export Ground.x Ground.obj` + `tools/obj2model.py`, exactly the pipeline
this task's brief suggested and already proven for InverseKinematics' own
`cylinder.x`) built and loaded without error but rendered **nothing at all** — a
solid CornflowerBlue screen with only 2D HUD text visible, confirmed via two
screenshots taken 3 seconds apart, no crash. This sample's own initial camera
distance is `sqrt(2000^2 + 3500^2) ≈ 4031` units
(`DesiredPositionOffset = (0, 2000, 3500)`) — even farther than Graphics3D's
~3523-unit spaceship, which showed the identical "fully invisible" symptom.

Worked around with a new `samples/ChaseCamera/src/RawModel.hpp` (NOXNA) — the
same bypass shape as InverseKinematics' `CylinderModel.hpp` and
HeightmapCollision's/GeneratedGeometry's `Terrain.hpp`: reads the
already-converted `_verts.bin`/`_idx.bin` sidecars directly and constructs real,
normally-initialized C++ `VertexPositionNormalTexture` objects (not a
`reinterpret_cast` on a raw byte blob), uploaded through the same typed
`VertexBuffer::SetData` overload `ModelTypeReader` was trying (and failing) to
reach — generalized this time to also bind a real `Texture2D` directly to the
`BasicEffect` (the same side benefit `Terrain.hpp` already established, since
`.model.json` has no per-mesh texture field). Confirmed live via screenshot:
both the ship (`Ship_p1_wedge_geo1`: 32458 vertices, 16118 triangles — two
orders of magnitude larger than InverseKinematics' 418-vertex cylinder) and the
ground (`Ground`: only 6 vertices, 2 triangles — the smallest mesh yet tested
through this bug) now render correctly, fully textured (`ShipDiffuse.png`/
`Checker.png`) and shaded. **This is a third and fourth independent
confirmation of DEFERRED.md item #26's hypothesis, on two more assets converted
through two different pipelines (FBX and `assimp` `.x`→`.obj`), at both a much
larger and a much smaller vertex count than the first confirmation** — stated
explicitly, as this task's brief requested, since it further reinforces that
this bug is a structural reader defect uncorrelated with mesh size, complexity,
or source format, not something specific to InverseKinematics' own cylinder
asset.

A second, separate (non-#26) issue surfaced after switching to `RawModel.hpp`:
the ship rendered correctly immediately, but the ground still didn't appear at
all. Isolated live by temporarily forcing `RasterizerState::CullNone` around
only the ground's draw call — confirmed this alone made the full checkered
ground plane appear, with no other change. Root cause: `assimp export
Ground.x Ground.obj` re-emits the `.x` file's two triangles wound the opposite
way from CNA's default `RasterizerState::CullCounterClockwise`, so the ground
was being fully back-face-culled even with correct, uncorrupted vertex data.
This is the same class of per-asset winding accommodation
`HeightmapCollision`'s/`GeneratedGeometry`'s own `Terrain.hpp` already needed
for their runtime-built terrain meshes — not a new bug, just the first time
it's shown up on an `assimp`-round-tripped `.x` asset specifically rather than
a hand-built runtime mesh. `Ship.fbx` (converted directly via
`tools/fbx_ascii2model.py`, no `assimp` round-trip) needed no such adjustment.
Kept as a permanent, documented `RasterizerState::CullNone`/
`CullCounterClockwise` toggle around only the ground's draw call in the final
port, not a global culling change.

Builds 0 warnings (verified via a from-scratch rebuild — object file removed,
rebuilt, output grepped for "warning"/"error", none found). Ran under
`SDL_VIDEODRIVER=x11` for 8+ seconds across three separate runs with no crash.
F1 help overlay and ship-movement/camera-spring-physics were both verified live
via this repo's established temporary debug-auto-trigger pattern (`helpTimer_`
forced to 10.0f and `Ship::Update()`'s `thrustAmount` forced to 1.0f, both
reverted before commit) — the help panel renders the correct 4-row control
table extracted from `ChaseCamera.htm`, and two screenshots 4 seconds apart show
the ship visibly moving forward with the chase camera visibly lagging/springing
behind it, confirming `ChaseCamera::Update()`'s ported spring-damper math
(`force = -Stiffness*stretch - Damping*velocity`) computes live, correct
results. `xdotool getactivewindow` showed a different real user window had
focus throughout this session (this repo's known shared-desktop caveat — section
5), so no synthetic keypresses were sent to the sample's own window. See
`samples/ChaseCamera/missing.md` for the complete account.

Commit this session: see git log for the exact hash, pushed to `develop`.

**Newest session (2026-07-10, follow-up):** Ported **InverseKinematics (#057)**,
section 8 task 6's last remaining candidate from the original 3-sample list
(ChaseCamera and MarbleMaze are now the only ones left — see section 8). This
session's real engineering content ended up being a significant, previously
undiagnosed CNA bug discovery, not the IK/CCD math itself (which is pure
application-level code, unrelated to any `Model`/bone-hierarchy machinery — the
task brief's concern about DEFERRED.md item #6's multi-bone gap turned out to be
a non-issue: the cylinder chain is 20 draws of one single-bone rigid model with
per-draw world matrices computed entirely in game code, and the sample's second
IK demo, driving an Xbox LIVE `AvatarRenderer`, is a real, working CNA type that
just faithfully never renders off a signed-in Xbox LIVE session, matching real
XNA/FNA exactly, not a CNA gap).

The actual discovery: a straightforward first port (`cylinder.x` →
`cylinder.model.json` via `assimp export` + `tools/obj2model.py`, the exact
pipeline already proven for CameraShake/PerformanceMeasuring/Graphics3D) built
cleanly and loaded without error (confirmed via debug instrumentation: 1 mesh,
418 vertices, 190 primitives, a correctly-linked `BasicEffect`) but **never
rendered**, at any camera distance, object scale, cull mode, or lighting
setting — even reduced to a single full-scale, identity-world, unlit, untextured
triangle drawn through the exact same `Model`/`ModelMesh::Draw()` path used by
every other Model-based sample in this repo. Systematic isolation (documented in
full in `samples/InverseKinematics/missing.md`) ruled out this sample's own code,
camera setup, and asset data one at a time: a hand-built triangle at the same
scale failed identically; **PickingSample's own already-shipped, already-working
`Cylinder.model.json`** failed identically when loaded fresh into this sample's
code; but **HeightmapCollision's `sphere.model.json` (3252 vertices) rendered
correctly** through the exact same code path at every scale tested (including
scaled down to match the failing tests' size) — the key data point that
something in the *reader itself*, correlated with mesh size/complexity, not any
one sample's code or asset, was responsible.

Root-caused by direct source read plus a standalone `sizeof()` probe compiled
against `cna`'s own headers: every CNA vertex struct
(`VertexPositionColor`/`VertexPositionTexture`/`VertexPositionColorTexture`/
`VertexPositionNormalTexture`) now publicly inherits from the polymorphic
`IVertexType` (a virtual destructor plus a pure virtual method), adding an
8-byte vtable pointer XNA's own "clean" layouts never had — confirmed live:
`sizeof(VertexPositionColor)=40` (not 16), `sizeof(VertexPositionTexture)=32`
(not 20), `sizeof(VertexPositionColorTexture)=56` (not 24),
`sizeof(VertexPositionNormalTexture)=40` (not 32).
`ModelTypeReader::Read()`'s `if (stride == sizeof(VertexPositionNormalTexture))
... else if (stride == sizeof(VertexPositionTexture)) ...`-style dispatch
compares the `.model.json`-declared *clean* stride (always 32 for every
`Content.Load<Model>`-based sample in this repo, since `obj2model.py`/
`fbx_ascii2model.py` only ever emit `VertexPositionNormalTexture` data) against
these now-inflated sizes — and `32` *accidentally* equals
`sizeof(VertexPositionTexture)`'s own inflated size (a coincidence of the
specific field-count/padding arithmetic), so the reader always silently
dispatches to the **wrong** overload, `reinterpret_cast`ing the raw (vtable-free)
file bytes as if they were vtable-shifted `VertexPositionTexture` objects and
reading Position/TextureCoordinate from the wrong byte offsets — corrupting the
uploaded vertex data for **every stride-32 `.model.json` in this entire repo**,
not just this sample's.

This is very likely the true root cause of the long-tracked
"near-plane-clipping-family" bug (section 4) — a corrupted, essentially
arbitrary per-vertex byte-offset reinterpretation would produce exactly the two
symptoms already observed (a degenerate thin line, or full invisibility)
without requiring any actual clip-space/projection defect, and no prior session
ever directly inspected the EasyGL clipping code itself before attributing the
symptom to it. **Not re-confirmed with 100% certainty for the other affected
samples this session** (would need re-testing `tank.model.json`/
`terrain.model.json` through the same bypass, out of this task's scope) — filed
as new DEFERRED.md item #26 with a clear recommendation for whoever picks up
section 8 task 2 next: try the bypass on CameraShake (the smallest affected
sample) before assuming the bug is really in clip-space math.

Worked around in this port with a new `samples/InverseKinematics/src/
CylinderModel.hpp` (NOXNA): reads the already-converted `cylinder_verts.bin`/
`cylinder_idx.bin` directly and constructs real, normally-initialized C++
`VertexPositionNormalTexture` objects field-by-field (not via
`reinterpret_cast`), then uploads them through the same typed
`VertexBuffer::SetData(const VertexPositionNormalTexture*, count)` overload
`ModelTypeReader` was trying (and failing) to reach — the same shape of
workaround already established by `HeightmapCollision`'s/`GeneratedGeometry`'s
own `Terrain.hpp` for a different `.model.json` gap, and further proof (since
their terrain already goes through this exact typed overload and renders
correctly) that the typed `SetData` path itself was never the problem.

Confirmed live via screenshot: the 20-link cylinder chain renders correctly,
**lit** (a visible shading gradient along the chain — this sample does not hit
the "flat white, no shading" finding from PickingSample/TrianglePicking/
HeightmapCollision, since `CylinderModel.hpp` bypasses the "no per-mesh texture
in `.model.json`" gap entirely by not using `.model.json` at all), correctly
curling from the origin toward the cat's live position every frame — direct
confirmation the CCD IK algorithm itself computes correct, live bone rotations.
The billboarded cat and HUD text render correctly. Builds 0 warnings; ran 9+
seconds with no crash across two separate runs. F1 help overlay verified via
this repo's established temporary-debug-auto-trigger pattern (`helpTimer_`
forced on, screenshotted, reverted before commit). This sample's own camera
sits only ~5 units from the action (`cameraRadius=5`, `near=1`, `far=1000`) —
much closer even than HeightmapCollision's ~155 units — and shows no near-plane
artifact at all, consistent with (though not proof of) the item #26
re-attribution above. See `samples/InverseKinematics/missing.md` for the
complete account.

Commit this session: `67219d6`, pushed to `develop`.

**Newest session (2026-07-10):** Ported **HeightmapCollision (#049)**, section 8
task 6's recommended next candidate — a rolling ball on a heightmap-generated
terrain, with both the ball and the follow-camera queried against a
`HeightMapInfo` height-lookup class (bilinear interpolation over the same height
grid the terrain mesh itself uses).

The real engineering question was the terrain. `HeightmapCollision.cs`'s own
runtime code is nothing more than `Content.Load<Model>("terrain")` + reading
`terrain.Tag as HeightMapInfo` — all the actual work happens at content-**build**
time, in `HeightmapCollisionPipeline.TerrainProcessor` (a custom
`ContentProcessor<Texture2DContent, ModelContent>` that reads `terrain.bmp` as a
heightfield, builds a grid mesh via `MeshBuilder`, chains to the stock
`ModelProcessor`, and attaches a `HeightMapInfoContent` — the same height data,
computed once — to the built model's `Tag`). CNA has neither a `Model.Tag`
equivalent nor custom-`ContentProcessor` extensibility (pre-existing DEFERRED.md
item #18), so this isn't directly portable. Rather than inventing a new
workaround, this session first read this repo's own `GeneratedGeometry` sample
(per the task brief's suggestion) and confirmed via its `missing.md` that its own
C# original ships a **structurally identical** `TerrainProcessor` — meaning
`GeneratedGeometry`'s existing runtime-mesh-generation approach
(`samples/GeneratedGeometry/src/Terrain.hpp`) is precedent, not just a candidate
idea. This session's `Terrain.hpp` (NOXNA) follows the same shape: builds the
terrain's `VertexBuffer`/`IndexBuffer`/`BasicEffect` **and** its `HeightMapInfo`
together at runtime in `LoadContent()`, replicating `TerrainProcessor.Process()`'s
exact algorithm (`terrainScale=30`, `terrainBumpiness=640`, `texCoordScale=0.1`)
and computing per-vertex normals from the heightfield's gradient in place of
`ModelProcessor`'s automatic normal generation.

A second, independent reason favored this approach over `Content.Load<Model>`,
found by direct source read rather than assumed: `terrain.bmp` is 257×257 =
66049 vertices, exceeding the 65535 limit of a 16-bit index buffer. Real XNA's
`ModelProcessor` automatically selects 32-bit indices for a mesh this large;
CNA's `.model.json` `ModelTypeReader` (`ContentManager.cpp`) hardcodes 16-bit
indices unconditionally, with no way to request 32-bit for any mesh regardless of
size — a genuine, narrower nuance of DEFERRED.md item #6 not previously
documented (added as a new addendum this session). Confirmed this is not a
general CNA limitation, though: `IndexBuffer`/`IIndexBufferBackend` (both EasyGL
and Vulkan) already fully implement `IndexElementSize::ThirtyTwoBits`
end-to-end — the gap is specifically `ModelTypeReader`'s hardcoded assumption.
Building the terrain's `IndexBuffer` directly with the real
`IndexBuffer(device, IndexElementSize::ThirtyTwoBits, ...)` constructor
sidestepped this cleanly, verified working live (the full 257×257 grid renders
with no visible artifacts).

A pleasant, unplanned side effect: because this terrain never goes through
`.model.json` (whose mesh schema has no per-mesh texture field — DEFERRED.md item
#6's PickingSample/TrianglePicking addendum, the "flat white" finding repeated
across multiple prior samples), its hand-built `BasicEffect` gets a real,
already-loaded `Texture2D` (`rocks.bmp`) bound directly via
`setTextureProperty()`. Confirmed live via screenshot: the terrain renders fully
textured, with a clearly visible shading gradient across its hills — the first
sample in this repo's Model-based lighting series to *not* hit the flat-white
gap. The **sphere** (`Content.Load<Model>("sphere")`, a plain ASCII FBX with the
same `-90,0,0` `PreRotation` node-transform shape LensFlare's `terrain.fbx` had —
handled automatically by the already-fixed `tools/fbx_ascii2model.py`) does still
render as a small flat white shape, exactly as expected and not re-diagnosed.

Also confirmed, by direct comparison against `NEXT.md`'s own tracked bug list:
this sample's camera sits only ~155 units from the ball
(`CameraPositionOffset = (0,40,150)`), noticeably closer than the ~1000+ unit
distances that trigger the tracked near-plane-clipping-family bug elsewhere in
this repo — screenshots confirm neither the ball nor the terrain shows that
artifact here. Not claiming the bug is fixed; this sample's own camera geometry
simply doesn't reach the distance where it's been observed to trigger. This
sample also adds no `GameComponent`s at all (unlike PickingSample/Graphics3D/
TrianglePicking's `Cursor`/`Checkbox`es), so DEFERRED.md item #23 is simply not
exercised here — noted in `missing.md` rather than silently skipped.

Builds 0 warnings (verified via the real build after fixing one small C++
language-constraint issue: `Vector3` has no `operator*=`, only
`operator*(Vector3, float)`, unlike C#'s `Vector3 *= float` — a one-line fix, not
a CNA gap). Ran 7+ seconds with no crash across two separate runs. F1 help
overlay uses the standard `tools/gen_help_png.py` path with no one-off variant
needed (`HeightmapCollision.htm`'s table has the standard 3 columns). See
`samples/HeightmapCollision/missing.md` for the complete account.

Commit this session: `e15c287`, pushed to `develop`.

**Previous session (2026-07-09, third same-day follow-up):** Ported
**TrianglePicking (#048)**, section 8 task 6's recommended next candidate — a
close sibling of PickingSample (#047, ported immediately before it this same
session): same original author, same table-of-4-objects scene, and (confirmed
via `cmp`) byte-identical source FBX assets (`Sphere.fbx`, `Cats.fbx`≡`Cats.FBX`,
`Cylinder.fbx`, `P2Wedge.fbx`≡`P2Wedge.FBX`, `table.FBX`). The key difference,
confirmed by reading `TrianglePickingSample_4_0/TrianglePickingSample/Game.cs`
directly rather than assuming: this sample replaces PickingSample's simpler
per-object `BoundingSphere`-only test with a real **per-triangle** ray
intersection (a hand-ported Moller-Trumbore ray-triangle test), a fast
bounding-sphere pre-test only used to decide whether the expensive per-triangle
test is worth running at all.

The real engineering question this session was where the per-triangle vertex
data comes from. XNA's original gets it from a custom content-pipeline
processor (`TrianglePickingProcessor`, a `ModelProcessor` subclass) that walks
the model's node tree at content-**build** time and attaches a flat
`Vector3[]` (plus a precomputed `BoundingSphere`) to `Model.Tag`. CNA has
neither a `Model.Tag` equivalent nor custom-`ContentProcessor` extensibility
(pre-existing DEFERRED.md item #18). The task brief suggested a fallback —
reading the data back from the model's already-loaded `VertexBuffer`/
`IndexBuffer` at runtime instead — but a full read of both classes' headers
(`cna/include/Microsoft/Xna/Framework/Graphics/{VertexBuffer,IndexBuffer}.hpp`)
confirmed **neither has a `GetData()` method of any kind**: every method on
both is a `SetData`/`SetDataRaw`/`SetDataWithOptions` upload path, never a
readback path. This is a real, narrower CNA gap than item #18 (a sample could
in principle re-derive picking data from an already-loaded `Model` with no
custom content pipeline involved at all, and still couldn't) — filed as **new
DEFERRED.md item #25**. Worked around at the tooling level, consistent with
this repo's own "convert once, offline" asset philosophy (item #18's framing):
extended `tools/fbx_ascii2model.py` (which already parses every mesh's raw
triangle/vertex data to build `.model.json`'s own buffers) with an optional
`--picking <output.bin>` flag that, from that same parsed data, additionally
emits a flat binary sidecar of triangle-expanded vertex positions per model —
read back directly by the C++ port (`samples/TrianglePicking/src/
TrianglePickingData.hpp`) at `LoadContent()` time, with
`BoundingSphere::CreateFromPoints()` (already implemented in CNA) computed over
it once and cached, reproducing exactly what `TrianglePickingProcessor` stores
in `Model.Tag` in the original.

A second, useful clarification surfaced while investigating whether this
sample would hit DEFERRED.md item #23 (`Game::DoInitialize()`'s
`ComponentAdded`-after-`Initialize()` timing gap) the same way PickingSample
and Graphics3D both did. Reading `Game.cs` directly showed this sample's
`Cursor` component is added from the **constructor**, not `Initialize()` —
and reading `cna`'s `Game.cpp` directly showed why that matters: the base
`Game::Initialize()` (not `DoInitialize()`) separately, unconditionally loops
over every component already present in `Components_` and calls each one's own
`Initialize()` directly, independent of `ComponentAdded` subscription timing
entirely. A component added in the constructor — long before
`DoInitialize()`/`Initialize()` ever run — is already present in `Components_`
by the time this loop executes, so it initializes correctly with **no
workaround needed**. Confirmed live (built and ran without the
`AddComponent()` pattern PickingSample/Graphics3D both required; `Cursor`'s
texture/`SpriteBatch` all load correctly, cursor renders in every screenshot).
Added this clarification directly to DEFERRED.md item #23's own text, since it
narrows the gap's actual trigger condition (order relative to the override's
own `base.Initialize()` call, not simply "constructor vs. `Initialize()`").

Also unexpectedly got a **real, non-debug-forced** live confirmation that the
whole picking pipeline works correctly end-to-end: an `xdotool mousemove
--window <id> <x> <y>` call issued earlier in the session (while checking this
repo's own known `xdotool` keyboard-focus reliability caveat) evidently moved
the real OS pointer over the window even without confirmed focus — a later,
fully clean screenshot (no debug code active) shows correct
`"Inside bounding sphere: P2Wedge, Cylinder"` text and a correct `"P2Wedge"`
per-triangle-picked name label under the cursor, reproduced identically across
two screenshots taken 4 seconds apart. Separately, a temporary debug
auto-trigger (removed before commit) confirmed the picked-triangle magenta
wireframe outline (`DrawPickedTriangle()`) also renders correctly. Confirmed
the already-tracked near-plane-clipping thin-line artifact on the `Sphere`
model (visible in every screenshot, since this sample's own C# original's
camera doesn't auto-rotate without input — unlike PickingSample's port, which
added its own continuous auto-rotation not present in either sample's actual
source) and the already-tracked flat-white-saturation finding (DEFERRED.md
item #6 addendum) — both immediately recognized as known, per this session's
own briefing, not re-diagnosed from scratch.

Builds 0 warnings (verified via a from-scratch rebuild); ran 7+ seconds with no
crash across multiple runs. F1 help overlay confirmed via a temporary debug
auto-trigger (removed before commit) — this sample's own `.htm` has the
standard 3-column controls table, so (unlike PickingSample) the stock
`gen_help_png.py` needed no one-off column-selection variant. See
`samples/TrianglePicking/missing.md` for the complete account.

Commit this session: `5bd30b0`, pushed to `develop`.

**Newest session (2026-07-09, second same-day follow-up):** Ported
**PickingSample (#047)**, section 8 task 6's recommended next candidate (one
of the 6 remaining unblocked lighting samples, chosen for this continuous
unattended porting session). Source: `Game.cs`/`Cursor.cs`/
`BoundingSphereRenderer.cs` — a table with 4 pickable objects (`Sphere`,
`Cats`, `P2Wedge`, `Cylinder`), mouse-ray picking via `Viewport.Unproject` →
`Ray` → `BoundingSphere.Intersects`, with per-model name labels drawn via
`Viewport.Project` + `SpriteFont.DrawString` when the ray hits. Ported
`Cursor` as a `DrawableGameComponent` (Windows/mouse branch only — Xbox/
Windows-Phone branches dropped, matching every other desktop-only port in
this repo) and `BoundingSphereRenderer` as an ordinary instance class (C#
static-class-with-extension-method → C++ instance member, a natural
language-constraint adjustment, not a behavior change).

All 5 source FBX files (`table.FBX`, `Sphere.fbx`, `Cats.FBX`, `Cylinder.fbx`,
`P2Wedge.FBX`) turned out to be plain ASCII FBX 6.1, converted directly with
`tools/fbx_ascii2model.py` — no binary-FBX/`ufbx` workaround needed this time
(unlike Graphics3D). One asset-naming quirk found and worked around:
`Game.cs` calls `Content.Load<Model>("Table")` (capital T) even though the
source file/content-item name is lowercase `table` — only worked in the
original because Windows content loading is case-insensitive; kept the
literal `"Table"` string in the C++ port and named the converted files to
match (`Table.model.json` etc.) instead of changing the source line. Also
found, via direct `Cats.FBX` inspection, that its only real mesh is a plain
box (`Box01`) — not a detailed cat model — matching the original asset
exactly, not a conversion bug.

Two CNA gaps surfaced, both already covered by existing DEFERRED.md items
(no new items needed), plus one confirmed sighting of the already-tracked
near-plane-clipping bug:

1. **`Game::DoInitialize()`'s component-lifecycle gap (DEFERRED.md item
   #23)** — this sample's original creates `Cursor` and adds it to
   `Components` from inside `Initialize()`, the same pattern Graphics3D hit
   first. Worked around with the identical `AddComponent()` helper pattern
   established there.
2. **`ModelMesh::ParentBone` is always `nullptr` for `.model.json`-loaded
   models** — confirmed via direct source read of `ModelTypeReader::Read()`:
   it builds one synthetic `"Root"` bone but never assigns any mesh's
   `ParentBone` to it (no setter even exists). Dereferencing
   `mesh.ParentBone.Index` directly (mirroring the C# source literally, as
   this sample's `DrawModel()`/`RayIntersectsModel()` both do) segfaulted
   immediately. This is the same root cause DEFERRED.md item #6's existing
   "multi-bone rigid-part" note already describes — not a new gap. Worked
   around with a small `BoneIndexOf(ModelMesh*)` helper that falls back to
   bone index 0, the exact same fallback CNA's own `Model::Draw()` already
   uses internally; correct here since every model in this sample is
   logically single-bone.
3. **New finding: untextured `BasicEffect` + default lighting renders every
   model as flat, fully-saturated white, with zero shading gradient at any
   camera angle** (confirmed via pixel sampling across several screenshots
   at different camera-rotation angles — not a one-off framing coincidence).
   Root-caused via direct source read of `EasyGLGraphicsBackend.cpp`'s
   `EnsureLit3DProgram()`: the lit fragment shader always multiplies by
   `texture(uTexture, vUV)`, falling back to an internal 1×1 *white* texture
   when none is bound — a no-op multiply. Combined with `BasicEffect`'s
   default white `DiffuseColor` (never overridden, since this sample's
   original relies entirely on its FBX-embedded material textures for color)
   and XNA's bright standard 3-point `EnableDefaultLighting()` rig, the lit
   result exceeds `(1,1,1)` for a broad range of normals and clips to solid
   white. This is a direct (if visually dramatic) consequence of the
   already-known "no per-mesh texture in `.model.json`" gap first flagged by
   LensFlare's `ground.png` note, not an independent new lighting bug — added
   the addendum to DEFERRED.md item #6 that LensFlare's own `missing.md`
   predicted a future sample would eventually need.

Also separately confirmed, at one particular camera-rotation angle, the
already-tracked near-plane-clipping thin-diagonal-line artifact
(`CameraShake`/`CustomModelClass`/`LensFlare`/`Graphics3D`'s bug family) on
one of the models — an independent, angle-*dependent* issue from the
angle-*independent* flat-white finding above, not a duplicate observation.

`BoundingSphereRenderer`'s wireframe circles were not confirmed visible in
any screenshot (toggling them off made no visible difference) — flagged in
`samples/PickingSample/missing.md` as unresolved (possibly occlusion/depth-
test related), not assumed to be a bug or assumed to be working.

Builds 0 warnings (verified via a from-scratch rebuild); ran 5+ seconds with
no crash across multiple runs. F1 help overlay verified via this repo's
established temporary-debug-auto-trigger pattern (removed before commit) —
renders correctly, using a one-off `gen_help_png.py` variant (same pattern as
MicrophoneEcho) to pick the "Windows" column instead of the tool's default
"Windows Phone" column from the sample's 4-column controls table. Live mouse-
driven cursor/name-label interaction was not exercised via synthetic input
this session (same `xdotool` reliability caveat noted throughout this repo).
See `samples/PickingSample/missing.md` for the complete account.

Commit this session: `c58652e`, pushed to `develop`.

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

**Update 2026-07-10: likely re-attributed — read DEFERRED.md item #26 before
investigating this as a clipping bug.** While porting InverseKinematics, a
different, concretely-confirmed bug was found: `ModelTypeReader::Read()`
(`ContentManager.cpp`) picks a vertex-upload code path by comparing a
`.model.json`'s declared (clean, XNA-sized) `"vertexStride"` against `sizeof()`
of CNA's own vertex structs — but every one of those structs now inherits from
the polymorphic `IVertexType`, inflating their real sizes by an 8-byte vtable
pointer (confirmed live: `sizeof(VertexPositionNormalTexture)` is 40, not the
clean 32 every conversion tool declares). `"vertexStride": 32` (used by every
`Content.Load<Model>`-based sample in this repo) *accidentally* collides with
`sizeof(VertexPositionTexture)`'s own inflated size (also 32), so the reader
always uploads every model's vertex data through the *wrong* typed overload,
`reinterpret_cast`-reading position/texcoord fields from the wrong byte offsets
— corrupting every stride-32 model's geometry repo-wide. A corrupted,
essentially arbitrary per-vertex reinterpretation is a far more complete
explanation for the exact two symptoms below (thin line / full invisibility)
than an actual clip-space defect, and no prior session investigating this bug
ever directly opened the EasyGL clipping code to confirm the "suspected cause"
bullet below. **Not confirmed with certainty for tank.model.json/
terrain.model.json specifically** (that would need re-running one of them
through the same bypass used in `samples/InverseKinematics/src/
CylinderModel.hpp` — a fast, cheap first step recommended before any further
clip-space investigation). See DEFERRED.md item #26 and `samples/
InverseKinematics/missing.md` for the full write-up. The rest of this section
is kept as originally written, for full historical context on how the symptom
was originally characterized:

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
- **Why it matters now:** 1 more sample (MarbleMaze — LensFlare, Graphics3D,
  PickingSample, TrianglePicking, HeightmapCollision, InverseKinematics, and
  ChaseCamera are now all ported, see section 8) is otherwise unblocked and
  portable, but **should not be assumed to render correctly** just because it
  builds — it needs its own screenshot check for this same artifact once
  ported (and, per the above, "renders nothing" is now just as suspect as "shows
  a thin line" — don't assume a blank frame means something else is wrong
  without checking camera distance against this bug first). PickingSample's and
  TrianglePicking's own ports each confirmed the thin-line symptom once more
  (see section 3); HeightmapCollision's own port, by contrast, confirmed neither
  symptom at all — its camera sits only ~155 units from its subject, well under
  the ~1000+ unit distances where the bug has been observed, a useful negative
  data point on the distance-dependence theory (task 2's own reasoning).
  InverseKinematics's own port also confirmed neither symptom (camera only ~5
  units away) but for a different reason than distance alone — see the
  DEFERRED.md item #26 update above; its cylinder chain bypasses
  `ModelTypeReader` entirely via `CylinderModel.hpp`, so it was never exposed to
  the corrupted-vertex-upload bug in the first place regardless of distance.
  **ChaseCamera's own port went further: it empirically confirmed (not just
  assumed) that its own Ship/Ground models hit the exact "fully invisible"
  symptom at its ~4031-unit camera distance, then confirmed the item #26 bypass
  (`RawModel.hpp`) fixes it completely** — a third and fourth independent
  confirmation of item #26's hypothesis, on two more assets at both size
  extremes (32458 vertices and 6 vertices) — see `samples/ChaseCamera/missing.md`.
  Also, PickingSample surfaced a separate, angle-independent
  "flat white, no shading" finding (also confirmed again by TrianglePicking) —
  don't conflate the two; see `samples/PickingSample/missing.md` and
  `samples/TrianglePicking/missing.md`.

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

- **CONFIRMED BUG, unfixed, likely MIS-DIAGNOSED as clipping — read DEFERRED.md
  item #26 first** — EasyGL near-plane clipping renders certain `Model`-based
  geometry (confirmed: `tank.model.json` at CameraShake's and CustomModelClass's
  ~1059-unit camera distance, `terrain.model.json` at LensFlare's) as a
  degenerate thin line instead of the model — **and, confirmed 2026-07-09 via
  Graphics3D, the same bug renders geometry as fully invisible at longer
  (~3523-unit) camera distances**, isolated by direct asset-swap testing to be
  the camera distance, not the asset or drawing code. See section 4 for full
  detail. **Update 2026-07-10:** a concretely-confirmed, different bug was found
  while porting InverseKinematics — `ModelTypeReader::Read()` uploads corrupted
  vertex data for every stride-32 `.model.json` (an `IVertexType` vtable
  inflates every CNA vertex struct's `sizeof()` past the clean XNA size the
  format declares, causing the reader to upload through the wrong typed
  overload) — which is a far more complete explanation for a thin
  line/full-invisibility symptom than an actual clip-space defect that no prior
  session ever directly confirmed by reading the clipping code itself. Not yet
  re-confirmed on `tank.model.json` specifically, but strongly suspected to be
  the same bug — see DEFERRED.md item #26 and try that fix before investigating
  `EasyGLGraphicsBackend.cpp`'s clipping path.
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

**Recently completed (2026-07-09):** LensFlare (#041), Graphics3D (#046),
PickingSample (#047), and TrianglePicking (#048), all screenshot-verified — see
section 3 for the full account of each, including the new DEFERRED.md items
(#22–#25), the item #6 addendum PickingSample added, and TrianglePicking's
clarification to item #23.

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

2. **Investigate the EasyGL near-plane clipping bug itself (section 4) — READ
   DEFERRED.md item #26 FIRST, try that fix before assuming clip-space math.**
   - Goal: get tank/terrain/spaceship models to render fully instead of
     degenerating to a thin line (moderate camera distance) or disappearing
     entirely (longer camera distance). **Strong new lead (2026-07-10, found
     while porting InverseKinematics):** `ModelTypeReader::Read()`
     (`ContentManager.cpp`) almost certainly uploads corrupted vertex data for
     every stride-32 `.model.json` in this repo, due to an `IVertexType` vtable
     inflating `sizeof()` of every CNA vertex struct past the clean XNA sizes
     the `.model.json` format declares — see DEFERRED.md item #26 for the full
     mechanism, confirmed via a live `sizeof()` probe and a working fix
     (`samples/InverseKinematics/src/CylinderModel.hpp`, which bypasses the
     reader and renders correctly where the equivalent `Content.Load<Model>`
     path rendered nothing). **Recommended first step, cheaper than opening
     `EasyGLGraphicsBackend.cpp` cold:** apply the same
     `ModelTypeReader::Read()` fix item #26 describes (compare declared stride
     against the *intended* clean XNA size, not `sizeof()` of the polymorphic
     struct) and re-run CameraShake — if the tank suddenly renders fully, this
     was never a clipping bug at all.
   - Files: `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`
     (`ModelTypeReader::Read()`, item #26's fix) first; only look at
     `cna/src/CNA/Internal/Backends/EasyGL/EasyGLGraphicsBackend.cpp`
     (clipping/projection path) if the item #26 fix doesn't resolve it.
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

6. **Port the last remaining unblocked lighting sample: MarbleMaze.**
   PickingSample (#047), TrianglePicking (#048), HeightmapCollision (#049),
   InverseKinematics (#057), and ChaseCamera (#058), previously in this list,
   are now all ported — see section 3.
   - Goal: same pattern as LensFlare/Graphics3D/PickingSample/TrianglePicking/
     HeightmapCollision/InverseKinematics/ChaseCamera — port using stock
     `Model`/`BasicEffect` (or a `RawModel.hpp`-style bypass if
     `Content.Load<Model>` renders nothing — see below), screenshot-verify,
     expect (per task 2 — **DEFERRED.md item #26, now independently confirmed
     FOUR times across InverseKinematics' and ChaseCamera's own assets, at
     vertex counts from 6 to 32458 — treat any stride-32 `.model.json` in this
     repo as presumptively affected, not just a hypothesis**) either the
     thin-line or fully-invisible symptom depending on camera distance; that
     alone is not a reason to suspect a new bug. Also expect the "flat white, no
     shading gradient" finding PickingSample/TrianglePicking/HeightmapCollision
     surfaced (DEFERRED.md item #6's addendum) on any model whose original
     relied on a texture for material color/shading contrast, **unless** you
     bypass `Content.Load<Model>` and bind a real `Texture2D` directly to the
     `BasicEffect` the way ChaseCamera's `RawModel.hpp`/HeightmapCollision's
     `Terrain.hpp` both do — in that case expect fully textured/shaded
     rendering instead. If a future sample's `.model.json` mesh ever exceeds
     65535 vertices, also expect the newer 16-bit-index-only nuance
     HeightmapCollision found (item #6's second addendum) — build that mesh
     directly at runtime with a real 32-bit `IndexBuffer` instead, the same way
     `Terrain.hpp` does, rather than routing it through `Content.Load<Model>`.
     **Recommended approach given item #26's now-strong confirmation record:**
     don't wait to see a blank screen before bypassing — go straight to a
     `RawModel.hpp`-style loader (read the `tools/obj2model.py`/
     `tools/fbx_ascii2model.py`-produced `_verts.bin`/`_idx.bin` directly,
     construct real `VertexPositionNormalTexture` objects field-by-field, upload
     via the typed `VertexBuffer::SetData` overload, bind a real `Texture2D`)
     for any mesh this sample needs to actually render, and only fall back to
     plain `Content.Load<Model>` if a quick empirical test shows it actually
     renders correctly (worth still doing the empirical test once, the way
     ChaseCamera's port did, for one more repo-wide data point — but budget for
     the bypass being needed).
   - Note: MarbleMaze has a much larger source tree (140 files); only a
     specific subdirectory (`Source/EX2_Polishing/End/`) is likely the actual
     port target per its `missing.md` — read that file first.
   - Files: new `samples/MarbleMaze/src/`; read `samples/MarbleMaze/missing.md`
     first.
   - Verify: `cmake --build cmake-build-debug --target MarbleMaze_cna_samples`,
     run under `SDL_VIDEODRIVER=x11`, screenshot.

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
  yet. This does **not** apply to the lit-`BasicEffect`-only samples (LensFlare,
  Graphics3D, PickingSample, TrianglePicking, HeightmapCollision,
  InverseKinematics, and ChaseCamera are now ported; MarbleMaze is the last one
  left — section 8, task 6) — those need no shader work.
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
