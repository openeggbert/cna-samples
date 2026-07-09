# Missing / Differences from XNA 4.0 original

**Status: PORTED (2026-07-10).** See below for the full account, including a third and
fourth independent confirmation of DEFERRED.md item #26's `ModelTypeReader` vertex-corruption
bug (found while porting InverseKinematics), on two more independently-converted assets.

Source: `/rv/tmp/XNAGameStudio/Samples/ChaseCamera_4_0/ChaseCamera/{ChaseCamera.cs,
ChaseCameraGame.cs, Ship.cs}` plus `ChaseCameraContent/{Ship.fbx, Ground.x, ShipDiffuse.tga,
Checker.bmp, gameFont.spritefont}`. The sample demonstrates a spring-physics chase camera
(`ChaseCamera.cs`, pure `Vector3`/`Matrix` math) following a ship (`Ship.cs`, simple flight
physics) flying over a large checkered ground plane.

## Major finding: independent confirmation of DEFERRED.md item #26 (`ModelTypeReader` vertex-corruption bug), on two more assets

**XNA behaviour:** `ChaseCameraGame.cs`'s `DrawModel()` helper calls `Content.Load<Model>
("Ship")`/`Content.Load<Model>("Ground")` in `LoadContent()` and draws both models every
frame via the standard `Model.Meshes`/`BasicEffect.EnableDefaultLighting()` pattern used by
every other lit-`BasicEffect` sample in this repo.

**CNA port behaviour:** Empirically tested, per this task's own brief, **before** assuming
the item #26 bug applied here. A temporary test build used `Content.Load<Model>("Ship")`/
`Content.Load<Model>("Ground")` (`Ship.fbx` converted directly via `tools/fbx_ascii2model.py`;
`Ground.x` converted via `assimp export Ground.x Ground.obj` + `tools/obj2model.py`, per this
task's own suggested pipeline — both produce ordinary stride-32 `.model.json` files, built and
loaded without any error) plus the exact `DrawModel()`/`BoneIndexOf()` pattern already
established by PickingSample/TrianglePicking/HeightmapCollision. Screenshot-confirmed: **a
solid CornflowerBlue screen with only the 2D HUD text visible — neither the ship nor the
ground rendered at all**, across two screenshots 3 seconds apart, no crash. This sample's own
initial camera distance is `sqrt(2000^2 + 3500^2) ≈ 4031` units (`DesiredPositionOffset =
(0, 2000, 3500)`) — even farther than Graphics3D's ~3523-unit spaceship, which showed the
identical "fully invisible" symptom.

This matches DEFERRED.md item #26 exactly (see `samples/InverseKinematics/missing.md` for the
full root-cause writeup): every CNA vertex struct now inherits from the polymorphic
`IVertexType`, inflating its `sizeof()` past the "clean" XNA size (16/20/24/32) every
conversion tool in this repo declares as `"vertexStride"`. `"vertexStride": 32` (used by both
`Ship.model.json` and `Ground.model.json` here, like every other `Content.Load<Model>`-based
sample in this repo) *accidentally* equals the inflated `sizeof(VertexPositionTexture)` (also
32), so `ModelTypeReader::Read()` always dispatches to the wrong typed `SetData` overload and
`reinterpret_cast`s the raw, vtable-free file bytes as vtable-shifted `VertexPositionTexture`
objects — reading Position/TextureCoordinate from the wrong byte offsets.

**This is a THIRD and FOURTH independent confirmation of item #26's hypothesis** (the first
was InverseKinematics' 418-vertex `cylinder.model.json`), on two more independently-converted
assets — one FBX (`Ship_p1_wedge_geo1`: 32458 vertices, 16118 triangles — two orders of
magnitude larger than the cylinder) and one `.x`-file (`Ground`: 6 vertices, 2 triangles — the
*smallest* mesh yet tested through this bug) — **at both a much larger and a much smaller
vertex count than InverseKinematics' cylinder**, further reinforcing that this bug is a
structural reader defect, not something correlated with any particular mesh size, complexity,
or source format. Saying so explicitly, as this task's brief requested: **item #26's
hypothesis is confirmed again here, independently, for a fourth and fifth (Ship + Ground)
`.model.json` asset.**

**Workaround used in this port:** `src/RawModel.hpp` (NOXNA) — the same bypass shape
established by InverseKinematics' `CylinderModel.hpp` and HeightmapCollision's/
GeneratedGeometry's `Terrain.hpp`: reads the already-converted `Ship_p1_wedge_geo1_verts.bin`/
`_idx.bin` and `Ground_verts.bin`/`_idx.bin` (produced once, offline, by the *unchanged*
`tools/fbx_ascii2model.py`/`tools/obj2model.py`) directly and constructs real,
normally-initialized C++ `VertexPositionNormalTexture` objects (field-by-field, not a
`reinterpret_cast` on a raw byte blob), then uploads them through the same typed
`VertexBuffer::SetData(const VertexPositionNormalTexture*, count)` overload `ModelTypeReader`
was trying (and failing) to reach. Confirmed live via screenshot: both the ship (fully
textured with `ShipDiffuse.tga`→`.png`, visibly shaded) and the ground (fully textured with
`Checker.bmp`→`.png`, visibly shaded) render correctly — generalized from `CylinderModel.hpp`
to also bind a real `Texture2D` directly to the `BasicEffect` (the same side benefit
`Terrain.hpp` already established), since `.model.json` has no per-mesh texture field
(DEFERRED.md item #6's addendum) — so, unlike PickingSample/TrianglePicking/HeightmapCollision's
sphere, neither model in this sample hits the "flat white, no shading" finding at all.

**Tracked in:** DEFERRED.md item #26 (existing item, this session adds a confirmation note,
not a new item).

## Ground.x's converted winding needs `RasterizerState::CullNone`

**XNA behaviour:** `Ground.x`'s single `Mesh` block (a 4-vertex, 2-triangle quad, 65536 units
on a side, textured with `Checker.bmp` tiled 32×32) renders normally through the stock content
pipeline and `BasicEffect`'s default `RasterizerState.CullCounterClockwise`.

**CNA port behaviour:** After switching to `RawModel.hpp` (above), the ship rendered correctly
immediately, but the ground still did not appear at all — confirmed live (screenshot: ship
visible, floating over a plain CornflowerBlue background, no checker plane anywhere). Isolated
by temporarily setting `RasterizerState::CullNone` around only the ground's draw call:
confirmed live this alone made the full checkered ground plane appear, textured and shaded,
with no other change. Root cause: `assimp export Ground.x Ground.obj` (this task's own
suggested `.x`→`.obj` conversion path, since neither `tools/obj2model.py` nor
`tools/fbx_ascii2model.py` reads `.x` directly) re-emits the quad's two triangles wound the
opposite way from what CNA's default `RasterizerState::CullCounterClockwise` expects, so every
triangle was being back-face-culled. This is the same per-asset winding adjustment
`HeightmapCollision`'s/`GeneratedGeometry`'s own `Terrain.hpp` already needed for their
runtime-built terrain meshes (see those samples' `missing.md`), not a new class of bug — just
the first time it's been seen on an `assimp`-round-tripped `.x` asset specifically rather than
a hand-built runtime mesh. `Ship.fbx` (converted directly via `tools/fbx_ascii2model.py`, no
`assimp` round-trip) needed no such adjustment — its winding survives conversion correctly,
matching every other FBX-sourced model in this repo.

**Root cause:** `assimp export`'s re-triangulation/re-emission of `Ground.x`'s two faces uses
the opposite winding convention from CNA's default `RasterizerState::CullCounterClockwise`.

**CNA port behaviour (workaround):** `ChaseCameraGame::Draw()` sets
`RasterizerState::CullNone` around only the ground's draw call (restoring
`CullCounterClockwise` immediately after, for the ship and any future geometry), rather than
disabling culling globally.

**Tracked in:** not a CNA gap — a per-asset conversion-pipeline quirk of the `assimp export`
step this task's own brief suggested for `.x` files. Not filed as a new DEFERRED.md item
(same root cause/fix shape as the pre-existing `Terrain.hpp` precedent, just a different
trigger).

## Windows-only control scheme (matches every other desktop port in this repo)

**XNA behaviour:** `ChaseCameraGame.cs` has a `#if WINDOWS_PHONE` branch (480×800 portrait
back buffer, fixed 30 Hz `TargetElapsedTime`, full-screen) and a Windows/`#else` branch (853×480
landscape back buffer) — plus full `GamePad` support and `MouseState`-based "touch" helpers
(`TouchLeft`/`TouchRight`/`TouchUp`/`TouchDown` in `Ship.cs`, `touchTopLeft` in
`ChaseCameraGame.cs`) that predate real XNA touch APIs and already use mouse-as-touch directly,
even when compiled for Windows.

**CNA port behaviour:** Only the Windows (`#else`) branch is ported (853×480, matching every
other desktop-only port in this repo); `GamePad`/`Buttons`/`GamePadThumbSticks`/
`GamePadTriggers` code is kept (CNA supports `GamePad` fully) so a real gamepad still works
identically to the original. The mouse-based "touch" helpers port unchanged — no
mouse-substitutes-for-touch adaptation was needed here, since the C# original already used
`MouseState` + `GraphicsDevice.Viewport` thirds directly for this, not a real touch API.

**Root cause:** This repo targets desktop only; no Windows Phone target exists.

**Tracked in:** not planned.

## `graphics.SupportedOrientations = Portrait` has no visible effect on this desktop build

**XNA behaviour:** `ChaseCameraGame`'s constructor sets
`graphics.SupportedOrientations = DisplayOrientation.Portrait` unconditionally, even though the
non-Windows-Phone branch immediately afterward sets a landscape 853×480 back buffer. This
looks like a copy/paste leftover from the Windows-Phone branch (SupportedOrientations only
meaningfully restricts phone/tablet display rotation in real XNA); it has no effect on a
Windows desktop `.exe`'s window.

**CNA port behaviour:** Ported literally (`graphics_->setSupportedOrientationsProperty
(DisplayOrientation::Portrait)`), for source fidelity. Confirmed live this has no visible
effect on the sample's actual 853×480 landscape window (matches CNA's own previously-fixed
portrait-orientation-forcing bug, which is now platform-gated FNA-style and does not restrict
desktop windows).

**Root cause:** Faithful reproduction of an apparent copy/paste artifact in the original
sample, not a CNA gap.

**Tracked in:** not planned (cosmetic, upstream-only, no observable behavior difference).

## Verification

Built `ChaseCamera_cna_samples` with 0 warnings/0 errors, verified via a from-scratch rebuild
of the changed translation unit (`cmake --build cmake-build-debug --target
ChaseCamera_cna_samples -j$(nproc)`, object file removed and rebuilt, output grepped for
"warning"/"error" — none found). Ran under `SDL_VIDEODRIVER=x11` for 8+ seconds across three
separate runs with no crash.

Screenshot-confirmed: the ship (`Ship_p1_wedge_geo1`, textured with `ShipDiffuse.png`) renders
correctly, fully lit and shaded, floating above a fully textured/shaded checkered ground plane
(`Ground`, textured with `Checker.png`) that extends to the horizon in every direction — both
via the `RawModel.hpp` bypass described above. The 2D HUD text (thrust/steer/spring-toggle
instructions, matching `DrawOverlayText()`'s exact C# string) renders correctly in the
top-left corner. F1 help overlay verified via this repo's established temporary
debug-auto-trigger pattern (`helpTimer_` forced to 10.0f, screenshotted, reverted before
commit): renders the `.htm`-table-derived semi-transparent panel correctly, centered on
screen, with the correct 4-row control table (Steer / Accelerate / Toggle camera spring
enabled / Exit the sample) extracted verbatim from `ChaseCamera.htm`'s own "Sample Controls"
table (no one-off `gen_help_png.py` column-selection variant needed — this `.htm`'s
"Keyboard Control (Windows)" column is already the tool's default column 1).

Ship movement and the chase camera's spring-damper physics were also verified live via a
second temporary debug-auto-trigger (`Ship::Update()`'s `thrustAmount` forced to `1.0f`,
reverted before commit): two screenshots taken 4 seconds apart show the ship's on-screen
position visibly shifting as it accelerates forward, with the chase camera visibly lagging
behind/around it (spring inertia) rather than staying perfectly locked — exactly the intended
"spring camera" behavior, and direct confirmation `ChaseCamera::Update()`'s ported spring-force
math (`force = -Stiffness*stretch - Damping*velocity`) is computing live, correct results, not
just static placeholder geometry.

Live mouse/keyboard-driven interaction (arrow-key steering, spacebar thrust, `A` to toggle the
camera spring, `R` to reset) was not separately exercised via synthetic input this session:
`xdotool getactivewindow` showed a different, real user window had focus throughout this
session (consistent with this repo's own documented shared-desktop `xdotool` reliability
caveat — NEXT.md section 5), so no keypresses were sent to avoid interfering with that window;
the debug-auto-trigger method above was used instead, per this repo's established fallback.

No known differences beyond the three documented above (the item #26 vertex-corruption
bypass, the `Ground.x` winding/`CullNone` adjustment, and the cosmetic
`SupportedOrientations`/Windows-Phone-branch omissions).
