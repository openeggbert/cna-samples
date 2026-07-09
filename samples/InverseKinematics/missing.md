# Missing / Differences from XNA 4.0 original

**Status: PORTED (2026-07-10).** See below for the full account, including a major CNA
rendering-pipeline bug found and worked around while porting this sample.

Source: `/rv/tmp/XNAGameStudio/Samples/InverseKinematics_4_0/InverseKinematics/
InverseKinematics/{IKSample.cs, Cat.cs}` plus `InverseKinematicsContent/{cylinder.x,
cat.tga, font.spritefont}`. The sample demonstrates the Cyclic Coordinate Descent (CCD)
inverse-kinematics algorithm two ways: a 20-link chain of cylinder models reaching for a
billboarded "cat" sprite, and the same CCD chain driving an Xbox LIVE avatar's arm/head
to reach for (and look at) the cat.

## Major finding: a real CNA `ModelTypeReader` bug, not an asset/conversion problem

**XNA behaviour:** `LoadCylinderModel()` calls `Content.Load<Model>("cylinder")` and the
20-link chain (`DrawBones()`) draws 20 instances of that model, each with its own world
transform and diffuse color, reaching toward the cat.

**CNA port behaviour:** A straightforward first port of this (loading `cylinder.model.json`
via `Content.Load<Model>("cylinder")`, converted from `cylinder.x` with
`assimp export cylinder.x cylinder.obj` + `tools/obj2model.py`, exactly the established
pipeline used successfully by CameraShake/PerformanceMeasuring/Graphics3D) **built and ran
with 0 warnings, loaded without error** (confirmed via debug instrumentation: 1 mesh, 1
part, 418 vertices, 190 primitives, a valid `BasicEffect`, all correctly linked) — but
**the model never appeared on screen**, at any camera distance or object scale, with
`RasterizerState::CullNone`, with lighting forced off, and even reduced to a single
full-scale, identity-world, unlit, un-textured triangle drawn through the exact same
`Model`/`ModelMesh::Draw()` path. A hand-built triangle at the same scale/distance and
**PickingSample's own already-shipped, already-working `Cylinder.model.json`** (loaded
fresh into this sample's own code, sidestepping any possibility this was an asset-specific
problem) **both failed to render the same way** — ruling out this specific mesh's own
data, the camera setup, and the render state as the cause. A large model
(HeightmapCollision's `sphere.model.json`, 3252 vertices) rendered *correctly* through the
exact same code path at every scale tested (including scaled down to the same size as the
failing tests), which was the key clue that something size/count-related in the reader
itself, not this sample's own code, was responsible.

**Root cause, confirmed by direct source read plus a standalone `sizeof()` probe:** every
vertex struct in CNA (`VertexPositionColor`, `VertexPositionTexture`,
`VertexPositionColorTexture`, `VertexPositionNormalTexture`) publicly inherits from the
polymorphic `Microsoft::Xna::Framework::Graphics::IVertexType`
(`virtual ~IVertexType() = default;` plus a pure virtual `getVertexDeclarationProperty()`),
so every instance carries an 8-byte vtable pointer that the original, "clean" XNA vertex
layouts never had. A tiny standalone program (`sizeof(T)` for each of the four types,
compiled against `cna`'s own headers) confirms this live:

```
sizeof(VertexPositionColor)         = 40   (XNA/clean size: 16)
sizeof(VertexPositionTexture)       = 32   (XNA/clean size: 20)
sizeof(VertexPositionColorTexture)  = 56   (XNA/clean size: 24)
sizeof(VertexPositionNormalTexture) = 40   (XNA/clean size: 32)
```

`ModelTypeReader::Read()` (`cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`,
the "Meshes" parsing loop) picks which typed `VertexBuffer::SetData` overload to call for
a given mesh by comparing the `.model.json`-declared `"vertexStride"` (always one of the
*clean* XNA sizes — 16/20/24/32 — since every conversion tool in this repo
(`tools/obj2model.py`, `tools/fbx_ascii2model.py`) writes the tightly-packed layout) against
`sizeof(...)` of these now-vtable-inflated structs:

```cpp
if (stride == static_cast<int>(sizeof(Graphics::VertexPositionColor)))        // 40
    vb->SetData(reinterpret_cast<const Graphics::VertexPositionColor*>(...), numVertices);
else if (stride == static_cast<int>(sizeof(Graphics::VertexPositionNormalTexture))) // 40
    vb->SetData(reinterpret_cast<const Graphics::VertexPositionNormalTexture*>(...), numVertices);
else if (stride == static_cast<int>(sizeof(Graphics::VertexPositionColorTexture)))  // 56
    vb->SetData(reinterpret_cast<const Graphics::VertexPositionColorTexture*>(...), numVertices);
else if (stride == static_cast<int>(sizeof(Graphics::VertexPositionTexture)))       // 32
    vb->SetData(reinterpret_cast<const Graphics::VertexPositionTexture*>(...), numVertices);
```

None of the declared "clean" stride values (16/20/24/32) equal their *intended* struct's
real (vtable-inflated) size any more. Worse, `"vertexStride": 32` — the value **every**
`Content.Load<Model>`-based sample in this entire repo uses, since `obj2model.py`/
`fbx_ascii2model.py` only ever emit `VertexPositionNormalTexture` (position+normal+
texcoord) data — *accidentally* equals `sizeof(VertexPositionTexture)` (also 32, by
coincidence of the specific field-count/padding math), so the reader always
silently dispatches to the **wrong** branch: it `reinterpret_cast`s the raw, vtable-free
file bytes as an array of (vtable-shifted) `VertexPositionTexture` objects and reads each
vertex's `.Position`/`.TextureCoordinate` fields from the **wrong byte offsets** in the
source buffer (offset 8 instead of 0 for Position, since a real `VertexPositionTexture`
object has its vtable pointer at offset 0) — corrupting every single vertex's position
data for every stride-32 `.model.json` in this whole repository, not just this sample's.

This is almost certainly the true root cause of the "near-plane-clipping-family" bug
tracked in DEFERRED.md/NEXT.md since CameraShake (the tank/terrain "thin diagonal line"
and Graphics3D's spaceship "full invisibility") — a corrupted, effectively-arbitrary
per-vertex byte-offset reinterpretation would produce exactly this class of degenerate
symptom (a thin line, or nothing at all), and is a far more mechanistically complete
explanation than an actual near-plane clipping defect, which was never confirmed by a
direct look at the clipping code itself in any prior session. **Not re-attributed with
100% certainty in this session** (that would need directly re-testing `tank.model.json`/
`terrain.model.json` through the same bypass used here, which is out of this task's
scope) but flagged prominently as the most likely explanation. See DEFERRED.md item #26
for the full write-up and the exact fix location.

**Workaround used in this port:** `src/CylinderModel.hpp` (NOXNA) reads
`cylinder_verts.bin`/`cylinder_idx.bin` (produced once, offline, by the *unchanged*
`tools/obj2model.py`) directly and constructs real, normally-initialized C++
`VertexPositionNormalTexture` objects (field-by-field, not a `reinterpret_cast` on a raw
byte blob), then uploads them through the *same* typed
`VertexBuffer::SetData(const VertexPositionNormalTexture*, count)` overload
`ModelTypeReader` was trying (and failing) to reach — this works correctly because the
vtable-inflation problem only bites when raw file bytes are blindly reinterpreted as a
polymorphic object; a real, compiler-constructed C++ object's field accesses are always
correct regardless of its `sizeof()`. This mirrors the exact shape of the workaround
`HeightmapCollision`'s `Terrain.hpp`/`GeneratedGeometry`'s `Terrain.hpp` already
established for a different `.model.json` gap (building a `VertexBuffer`/`IndexBuffer`/
`BasicEffect` directly at runtime instead of via `Content.Load<Model>`) — confirmed live,
by that same precedent, that the typed `SetData` path itself works correctly (their
terrain renders fine through it). `cylinder.model.json` itself is left checked in
(documents the intended, eventually-usable asset format once the reader bug above is
fixed) but is not currently loaded by this sample.

**Tracked in:** DEFERRED.md item #26 (new).

## Avatar rendering renders nothing (not a CNA gap)

**XNA behaviour:** `IKSample.cs`'s second IK demo drives an `AvatarRenderer`/
`AvatarDescription`/`AvatarBone`/`AvatarExpression` (Xbox LIVE avatar) chain
(`LoadAvatar()`, `UpdateAvatarIK()`, `DrawAvatar()`), in parallel with the cylinder chain.

**CNA port behaviour:** Ported faithfully (real `AvatarRenderer`, `AvatarDescription`,
`AvatarBone`, `AvatarExpression` types, matching the C# original's structure exactly), but
renders nothing, because `AvatarRenderer::getStateProperty()` always returns
`AvatarRendererState::Unavailable` — confirmed by direct source read
(`cna/src/Microsoft/Xna/Framework/GamerServices/AvatarRenderer.cpp`): neither constructor
ever reads its `AvatarDescription` argument, and `state_` is forced to `Unavailable` on
every single read, unconditionally. The C# original's own `UpdateAvatarIK()`/`DrawAvatar()`
already defensively check `State != AvatarRendererState.Ready` before doing any real work
— this port preserves that guard exactly (`UpdateAvatarWorldTransforms()`/`AvatarLookAt()`/
the CCD loop in `UpdateAvatarIK()`, and the `DrawBones()` call in `DrawAvatar()`, are all
kept, compiled, and reachable in principle, but their guard conditions are never satisfied
in practice) — so the avatar half legitimately renders nothing, matching a real Windows
XNA build with no Xbox LIVE/Games-for-Windows-Live avatar signed in. This is not a CNA
gap, and not something a real, unmodified Windows build of this exact sample would show
differently either (the real Xbox Avatar body mesh/texture/animation data was always
streamed at runtime from Xbox LIVE servers that have been offline for over a decade — see
`cna/docs/avatar-real-rendering-ext.md`'s own framing of this same fact).

**Root cause:** Faithful reproduction of real XNA/FNA's own off-Xbox `AvatarRenderer`
behavior — not a bug.

**Tracked in:** not planned (working as intended/faithful).

## Documentation/code mismatch in the original sample (upstream, not a port issue)

**XNA behaviour:** `InverseKinematics.htm`'s own "Sample Controls" table lists **SHIFT**
as the keyboard control for "Play/Pause the simulation."

**CNA port behaviour:** `IKSample.cs`'s actual code checks `Keys.P` (`IsTriggered(Keys.P)`)
for this action, and its own on-screen HUD text says `"Press 'P' to toggle"` — matching
the code, not the `.htm`. This port implements the **actual code behavior** (`Keys::P`),
consistent with every other on-screen/behavioral detail in this port, and the F1 help
overlay (auto-generated verbatim from the `.htm`'s own table, per this repo's standard
`tools/gen_help_png.py` process) therefore shows "SHIFT" — an upstream inconsistency in
the original sample's own documentation, not something introduced by this port.

**Root cause:** Pre-existing mismatch between `InverseKinematics.htm` and
`IKSample.cs` in the original XNA sample.

**Tracked in:** not planned (cosmetic, upstream-only).

## Windows-only control scheme (matches every other desktop port in this repo)

**XNA behaviour:** `IKSample.cs` has separate Xbox (`DrawXboxSpecificHUD`) and Windows
(`DrawWindowsSpecificHUD`) HUD/control branches, selected by an `#if XBOX` compile-time
switch, plus full `GamePad`/gamepad-trigger support alongside keyboard/mouse.

**CNA port behaviour:** Only the Windows keyboard branch is ported (matching every other
desktop-only port in this repo); `GamePad`/`Buttons`/`GamePadThumbSticks`/
`GamePadTriggers` code is kept (CNA supports `GamePad` fully) so a real gamepad still
works identically to the original, but the Xbox-specific on-screen HUD text branch is
omitted.

**Root cause:** This repo targets desktop only; no Xbox 360 target exists.

**Tracked in:** not planned.

## Verification

Built `InverseKinematics_cna_samples` with 0 warnings/0 errors (`cmake --build
cmake-build-debug --target InverseKinematics_cna_samples -j$(nproc)`, verified via a
from-scratch rebuild of the changed translation units). Ran under
`SDL_VIDEODRIVER=x11` for 9+ seconds across two separate runs with no crash.
Screenshot-confirmed: the 20-link cylinder chain renders correctly, lit (visible shading
gradient along the chain, confirming `BasicEffect.EnableDefaultLighting()` +
`PreferPerPixelLighting` are both genuinely active — this sample does **not** hit the
"flat white, no shading" finding from PickingSample/TrianglePicking/HeightmapCollision,
since `CylinderModel.hpp` bypasses the "no per-mesh texture in `.model.json`" gap
entirely by not using `.model.json` at all), correctly curling from the origin toward the
billboarded cat's current position every frame — direct visual confirmation the CCD IK
algorithm itself is computing correct, live bone rotations, not just static placeholder
geometry. The billboarded cat (`cat.png`, converted from `cat.tga`) renders correctly,
always facing the camera. HUD text (simulation state, controls) renders correctly. F1
help overlay verified via this repo's established temporary-debug-auto-trigger pattern
(`helpTimer_` forced on, screenshotted, reverted before commit) — renders the
`.htm`-table-derived panel correctly, centered, semi-transparent.

This sample's own camera sits only ~5 units from the action (`cameraRadius = 5`, `near =
1`, `far = 1000`) — far closer than the ~1000+ unit distances previously associated with
the (now-reattributed, see above) near-plane-clipping-family symptom, and much closer
even than HeightmapCollision's own ~155-unit camera. No near-plane artifact of any kind
was observed here, consistent with (though not proof of) the DEFERRED.md item #26
re-attribution above.

Live mouse-driven interaction (this sample only uses keyboard/gamepad, not mouse,
matching the C# original exactly aside from the dropped Xbox branch) was not separately
exercised via synthetic input this session, consistent with this repo's own documented
`xdotool` reliability caveat (NEXT.md section 5); F1 alone was confirmed via the
debug-auto-trigger method described above since a live `xdotool key F1` attempt, as
expected, had no observable effect.
