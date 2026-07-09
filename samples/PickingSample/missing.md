# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-09.** Builds with 0 warnings. Ported using CNA's stock
`Model`/`BasicEffect`/`DrawableGameComponent`, `Viewport::Project`/`Unproject`,
`Ray`/`BoundingSphere::Intersects`, all with no CNA-side API gaps for the sample's
own picking logic (the historical blocker this sample was originally written
against — see DEFERRED.md item #5 — is resolved for `Model`-based rendering).
Confirmed live for 5+ seconds with no crash; F1 help overlay confirmed via a
temporary debug auto-trigger (removed before commit). Two real rendering
findings this session, both pre-existing CNA framework gaps, not introduced by
this port — see below.

Source: `/rv/tmp/XNAGameStudio/Samples/PickingSample_4_0/Picking/
{Game.cs, Cursor.cs, BoundingSphereRenderer.cs}`. (`GeometricPrimitive.cs` in
the same directory is dead code — not included in `Picking (Windows).csproj`'s
`<Compile>` list, confirmed via direct project-file read — and was not ported.)

## Asset conversion: 5 FBX models, all plain ASCII, converted directly
**XNA behaviour:** `Content.Load<Model>("Sphere"/"Cats"/"P2Wedge"/"Cylinder")`
and `Content.Load<Model>("Table")` load `Sphere.fbx`/`Cats.FBX`/`P2Wedge.FBX`/
`Cylinder.fbx`/`table.FBX` via the FBX content-pipeline importer.
**CNA port behaviour:** all 5 files are ASCII FBX 6.1 (confirmed via `file`),
converted directly with `tools/fbx_ascii2model.py` — no `assimp`/Blender
intermediate step needed (unlike Graphics3D's old binary FBX). `table.FBX` has
5 meshes (`TableTop`/`BackLeftLeg`/`BackRightLeg`/`FrontLeftLeg`/
`FrontRightLeg`); the other 4 files have exactly 1 mesh each.
**Root cause:** N/A — straightforward conversion, no CNA gap.
**Tracked in:** not planned.

## `Cats.fbx`'s only mesh is a plain box, not a detailed cat model
**XNA behaviour:** `Content.Load<Model>("Cats")` loads whatever geometry
`Cats.FBX` actually contains.
**CNA port behaviour:** confirmed via direct grep of `Cats.FBX`'s `Model:`
blocks — the file's only real geometry is a single mesh named `Box01` (36
vertices, 12 triangles, a plain rectangular box, roughly 0.2×0.2×0.2 units).
There is no separate cat-shaped mesh anywhere in the file (the other `Model:`
entries are all `Camera`/`CameraSwitcher` nodes, correctly excluded by the
converter). This is simply what the source asset is, not a conversion
omission — the sample's own thumbnail (`Picking/GameThumbnail.png`) shows a
simple box-like shape at the "Cats" position too, tinted by its own material,
consistent with a placeholder box, not a cat sculpture.
**Root cause:** N/A — matches the original asset exactly.
**Tracked in:** not planned.

## `Table` asset name case mismatch (Windows case-insensitivity vs. Linux)
**XNA behaviour:** `Game.cs:142` calls `Content.Load<Model>("Table")` (capital
T), even though the source file is `table.FBX` (lowercase) and the content
project's own `<Name>` override for it is also lowercase `table`
(`PickingContent.contentproj`). This only worked in the original because
Windows/NTFS content-pipeline output paths are case-insensitive.
**CNA port behaviour:** kept the literal `Content.Load<Model>("Table")` call
in `PickingGame.hpp` unchanged (matching the C# source exactly) and named the
converted asset files `Table.model.json`/`Table_*_verts.bin`/`Table_*_idx.bin`
(capital T) to match, since CNA runs on a case-sensitive filesystem.
**Root cause:** N/A — cosmetic asset-naming accommodation for a
case-sensitive filesystem, not a CNA gap.
**Tracked in:** not planned.

## Textures present in source assets but not bindable through `.model.json` (CNA gap, already tracked — DEFERRED.md item #6 addendum added this session)
**XNA behaviour:** `wood.tga` (table top/legs), `cat.tga` (Cats box), and
`wedge_p2_diff_v1.tga` (P2Wedge) are referenced as embedded material textures
inside their respective FBX files (not explicit `<Compile>` items in
`PickingContent.contentproj` — the FBX importer resolves them implicitly) and
are bound automatically to each mesh's `BasicEffect.Texture`/`TextureEnabled`
by the `ModelProcessor` at content-build time. The sample's own thumbnail
(`GameThumbnail.png`) shows a brown wood-grain table and colorfully-tinted
objects on it.
**CNA port behaviour:** converted all 3 `.tga` files to PNG (`wood.png`,
`cat.png`, `wedge_p2_diff_v1.png`, via Pillow) and copied them into `Content/`
for completeness, but **none of them are actually bound to any mesh** — CNA's
`.model.json` "meshes" schema (`ModelTypeReader::Read()` in
`ContentManager.cpp`) has no `"texture"` field at all (confirmed by direct
source read), exactly the same limitation already documented in
`samples/LensFlare/missing.md` for `ground.png`. Every mesh in this sample
therefore renders with `BasicEffect.TextureEnabled == false` and a plain white
`DiffuseColor` (the class default), not the textured/tinted look in the
original screenshot.
**Root cause:** `ModelTypeReader::Read()`'s simple mesh schema doesn't parse a
texture reference — a pre-existing CNA gap, not new.
**Tracked in:** DEFERRED.md item #6. LensFlare's own `missing.md` predicted
this would eventually need "a small addendum there if a future sample
specifically needs a textured static model" — this is that sample; added the
addendum this session (see the "New finding" entry below for why the visual
consequence is far more dramatic here than for LensFlare's terrain).

## New finding: untextured `BasicEffect` + default 3-point lighting renders every model as a flat, fully-saturated white shape with zero visible shading gradient
**XNA behaviour:** every mesh renders with its own material texture (wood
grain / cat pattern / wedge diffuse — see above) modulating the lit result,
producing visible color and shading contrast, as seen in the original's
thumbnail.
**CNA port behaviour:** confirmed live via screenshot at several different
camera-rotation angles (not just one) — every model (`Sphere`, `Cylinder`,
`Table`, etc.) renders as a **perfectly flat, fully-saturated white shape**:
sampling pixels across the visible silhouette shows literally every point at
exactly `(255,255,255)`, with a razor-sharp (non-antialiased) edge against the
CornflowerBlue background and *zero* brightness variation anywhere, at every
camera angle checked. Confirmed this is **not** the already-tracked
near-plane-clipping thin-line/invisible bug (section 4/5 of `NEXT.md`) — that
bug *was* also separately observed once in this same session (a model showing
the familiar thin-diagonal-line artifact at one particular camera angle,
consistent with the known family) — but the flat-white saturation is a
distinct, additional phenomenon confirmed independent of camera angle.
Root-caused (without attempting a fix, per this repo's convention) via direct
source read of `EasyGLGraphicsBackend.cpp`'s `EnsureLit3DProgram()` fragment
shader: for `VertexPositionNormalTexture` (stride 32) draws, the shader
**always** samples `uTexture` and multiplies it into the lit result
(`FragColor = texture(uTexture, vUV) * vec4(litRGB, uDiffuseColor.a)`), with
no branch for "no texture bound." When no texture is bound (this sample's
case), `BindDrawParams` correctly falls back to an internal 1×1 white texture,
so the multiply is effectively a no-op and `FragColor.rgb` == `litRGB`
directly. With `BasicEffect.DiffuseColor` at its default `(1,1,1)` (never
overridden by this sample, matching the C# original, which relies entirely on
the texture for material color) and XNA's standard `EnableDefaultLighting()`
rig (ambient ≈ 0.05–0.18 + up to 3 directional lights each contributing up to
~0.3–1.0 per channel), `litRGB` exceeds `(1,1,1)` for a broad range of surface
normals facing the combined lights, and OpenGL clamps the framebuffer output to
`[0,1]` — the same clamping real XNA/DirectX would also apply. **This means a
correctly-textured mesh would look right** (a wood/cat/wedge-colored texture
with a typical average brightness well under 1.0 would pull the product back
under the clamp for most pixels, restoring visible shading contrast, exactly
as seen in the original screenshot) — **the flat-white appearance is a direct,
visually dramatic consequence of the missing per-mesh texture support above
(DEFERRED.md item #6), not a separate, new lighting bug.** LensFlare's own
`ground.png` note anticipated this exact scenario ("worth a small addendum
[to item #6] if a future sample specifically needs a textured static model")
— this is confirmed to be that case, with a much more visually severe result
than LensFlare's terrain (which is duller/more subtly affected, not fully
saturated) because this sample's models rely on their textures for both color
*and* practically all of their visible shading contrast, given a plain-white
default material.
**Root cause:** same underlying gap as the entry above (no per-mesh texture in
`.model.json`); the lit-shader's lack of a texture-less code path is a
contributing detail but not an independent bug, since its white-texture
fallback is mathematically a no-op multiply, not a broken computation.
**Tracked in:** DEFERRED.md item #6 (addendum added this session).

## Near-plane-clipping-family bug also observed (matches the already-tracked pattern)
**Confirmed live:** at one particular camera-rotation angle, one of the
on-table models rendered as the familiar thin, near-vertical dashed/diagonal
line instead of a solid shape — the same artifact already tracked for
`CameraShake`/`CustomModelClass`/`LensFlare`/`Graphics3D` (`NEXT.md` section
4/5). This is a separate, independent framework issue from the flat-white
finding above (that one is angle-*independent* and affects every model's
*fill*; this one is angle-*dependent* and affects a model's *geometry*/
clipping). Not attempted to fix here, per this repo's convention of tracking
framework rendering bugs centrally rather than re-diagnosing them per sample.
**Tracked in:** `NEXT.md` section 4/8 (task 2), pre-existing; no new
DEFERRED.md item needed.

## `BoundingSphereRenderer`'s wireframe circles not visibly confirmed on screen
**XNA behaviour:** `BoundingSphereRenderer.Draw()` renders 3 colored
(blue/red/green) wireframe circles (one per axis-aligned plane) around each
picked model's world-space bounding sphere, via a dedicated unlit
`BasicEffect` (`LightingEnabled = false`, `VertexColorEnabled = true`) and
`GraphicsDevice.DrawPrimitives(PrimitiveType.LineList, ...)`.
**CNA port behaviour:** ported the same logic essentially verbatim (see the
static-class → instance-class note below) — confirmed live that toggling
`drawBoundingSphere_` off makes no visible difference to any screenshot taken
this session, meaning the wireframe circles are not currently visible on
screen (they may be occluded by the solid-white model geometry's own depth
values, since a bounding sphere's radius is only marginally larger than its
mesh in most of these cases, or there may be a separate issue specific to this
`LineList` draw path). Not root-caused further this session — flagged here
rather than assumed to be working, since it could not be confirmed via
screenshot either way.
**Root cause:** not determined.
**Tracked in:** not filed as a DEFERRED.md item — would need isolated
follow-up investigation (e.g. a dedicated single-sphere test) before that's
warranted; may simply be an occlusion/depth-test interaction rather than a
bug, and doesn't block the sample's own build/run.

## `ModelMesh::ParentBone` is always `nullptr` for `.model.json`-loaded models (pre-existing CNA gap, worked around the same way CNA's own `Model::Draw()` does)
**XNA behaviour:** `mesh.ParentBone.Index` always resolves to a valid bone
index (`Game.cs` uses this directly in `DrawModel()`/`RayIntersectsModel()` to
look up each mesh's absolute bone transform).
**CNA port behaviour:** confirmed via direct source read of
`ModelTypeReader::Read()` (`ContentManager.cpp`) — it always builds exactly
one synthetic `"Root"` `ModelBone`, but never assigns it (or any bone) as a
mesh's `ParentBone`; `ModelMesh` doesn't even expose a setter for that field.
Dereferencing `mesh->getParentBoneProperty()->getIndexProperty()` directly
(mirroring the C# source literally) therefore segfaults immediately.
**Workaround applied:** added a small `BoneIndexOf(ModelMesh*)` helper that
falls back to bone index `0` when `getParentBoneProperty()` is `nullptr` —
the exact same fallback CNA's own `Model::Draw()` already uses internally
(confirmed by reading `Model.cpp`). Since every model in this sample is
logically single-bone (`CopyAbsoluteBoneTransformsTo` always produces exactly
one `"Root"` transform, matching the one bone `ModelTypeReader` ever creates),
index `0` is exactly the correct fallback, not a guess.
**Root cause:** same pre-existing gap already described in DEFERRED.md item
#6's "multi-bone rigid-part" note (`ModelMesh` has no per-mesh parent-bone
setter) — not new, just newly hit by a sample that dereferences `ParentBone`
directly instead of going through `Model::Draw()`'s own internal guard.
**Tracked in:** DEFERRED.md item #6 (existing note already covers this; no
new item needed).

## `BoundingSphereRenderer`: C# static class → C++ instance class
**XNA behaviour:** `BoundingVolumeRendering.BoundingSphereRenderer` is a
`static` class holding shared GPU state (one `VertexBuffer` + one
`BasicEffect`), with a C# extension method (`this BoundingSphere sphere`) for
`Draw()`.
**CNA port behaviour:** ported as an ordinary instance class
(`PickingSample::BoundingSphereRenderer`), held as a private member of
`PickingGame` — C++ has no direct equivalent of a C#-style static class with
extension methods, and this sample only ever needs one instance, so this is a
natural, behavior-preserving adjustment (per `CLAUDE.md`'s "within the natural
constraints of the C# → C++ language difference"), not a functional change.
`Initialize()`/`Draw()` do exactly what the original's same-named static
methods did.
**Root cause:** N/A — C++ language-constraint adjustment.
**Tracked in:** not planned.

## `Game::DoInitialize()` component-lifecycle workaround (DEFERRED.md item #23)
**XNA behaviour:** `Game.cs:Initialize()` creates `cursor = new Cursor(this);`
and calls `Components.Add(cursor);` from inside its own `Initialize()`
override, before calling `base.Initialize()` — supported in real XNA/FNA
because `Game`'s constructor already subscribed to `Components.ComponentAdded`
before any override can run.
**CNA port behaviour:** ported the same structure (`Cursor` created and added
to `Components` from inside `PickingGame::Initialize()`), which hits the same
already-documented CNA gap Graphics3D found first: `Game::DoInitialize()`
wires up `ComponentAdded` only *after* calling the user's `Initialize()`
override, so a component added from within `Initialize()` never gets its own
`Initialize()`/`LoadContent()` called automatically. Worked around with the
same `AddComponent(Cursor*)` helper pattern established in
`samples/Graphics3D/src/Graphics3DGame.hpp` — calls `Components.Add(...)`
followed by an explicit `component->Initialize()`.
**Root cause:** `cna`'s `Game::DoInitialize()` (`Game.cpp`) subscribes
`Components_.ComponentAdded`/`ComponentRemoved` after calling `Initialize()`,
not before.
**Tracked in:** DEFERRED.md item #23 (pre-existing, not new — this sample is
the second, after Graphics3D, to hit it).

## Windows-only input branch kept; Xbox/Windows-Phone branches dropped
**XNA behaviour:** `Cursor.cs`'s `Update()` is `#if XBOX` / `#elif WINDOWS` /
`#elif WINDOWS_PHONE`-gated: gamepad thumbstick+D-pad on Xbox, mouse on
Windows, first touch point on Windows Phone. `Game.cs`'s own `Update()` also
checks `GamePad.GetState(PlayerIndex.One).Buttons.Back` alongside keyboard
Escape for exit, unconditionally on every platform.
**CNA port behaviour:** kept only the `WINDOWS` (mouse) branch in `Cursor.hpp`
— confirmed via `Picking.htm`'s own "Sample Controls" table that this is a
desktop/Windows sample (`Mouse` moves the cursor; the "Windows/Xbox Gamepad"
column is empty for the "Move the cursor" row in the port's actual controls,
meaning there's no gamepad-driven cursor movement documented for this port to
preserve beyond what's already an XBOX-only code branch in the original).
Kept the gamepad-Back-to-exit check verbatim (unconditional in the original,
ported unconditionally here too, via `GamePad::GetState(PlayerIndex::One)`).
**Root cause:** N/A — platform branch selection, not a CNA gap (matches the
original's own already-existing Windows branch, not an invented substitution).
**Tracked in:** not planned.

## F1 help overlay controls table: picked the "Windows" column, not "Windows Phone"
**XNA behaviour:** N/A (CNA-only addition per `CLAUDE.md`).
**CNA port behaviour:** `Picking.htm`'s Sample Controls table has 4 columns
(Action | Windows Phone | Windows | Windows/Xbox Gamepad) — `tools/
gen_help_png.py` defaults to column 1 ("Windows Phone" — `TAP`/`DRAG` screen),
which doesn't match this desktop port's actual mouse-driven controls.
Generated `Content/help.png` with a one-off variant script (same established
pattern as `MicrophoneEcho`, see its `missing.md`) that reads column 2
("Windows": `Mouse` / `ESC or ALT+F4`) instead.
**Root cause:** `gen_help_png.py`'s column-selection is hardcoded to index 1;
not generalized in the shared tool (out of scope for a single sample port).
**Tracked in:** not planned (worked around per-sample, not a CNA gap).

## `Background.png` (leftover source asset) not converted
**XNA behaviour:** `Picking/Background.png` sits in the sample's project
folder but is referenced by **neither** `PickingContent.contentproj` nor
`Picking (Windows).csproj` (confirmed via grep of both project files) — it is
not loaded by `Game.cs` and not part of the content build at all.
**CNA port behaviour:** not converted/copied — genuinely dead/unused source
art, not a missed asset.
**Root cause:** N/A — leftover file in the original sample tree.
**Tracked in:** not planned.

## `DrawModelNames()` cursor-over-model text labels not interactively confirmed
**XNA behaviour:** when the cursor ray intersects a model's bounding sphere,
that model's name is drawn above it via `SpriteBatch.DrawString` with a
drop-shadow effect.
**CNA port behaviour:** ported verbatim (`Viewport::Project`,
`SpriteFont::MeasureString`, the same drop-shadow double-draw). The rendering
path (font load, `DrawString` calls, `RayIntersectsModel`) is present and
builds/runs without error, but moving the real mouse over a model to trigger
the label was not exercised via synthetic input this session — same
`xdotool` reliability caveat noted elsewhere in this repo (`NEXT.md` section
5). The mouse cursor icon itself (`cursor.png`) is confirmed rendering
correctly in every screenshot this session, tracking the X11 pointer's real
position (visible as the small triangular sprite in the screenshots' top-left
corner).
**Root cause:** N/A — not exercised, not a known bug.
**Tracked in:** not planned; a future session with reliable local input should
move the mouse over each model and confirm its name label appears correctly
before removing this note.

## Verification
**Confirmed live:** built `PickingSample_cna_samples` cleanly (0 warnings,
verified via a from-scratch rebuild). Ran under `SDL_VIDEODRIVER=x11` — window
opens (800×480), process stays alive 5+ seconds with no crash and no error
output beyond normal EasyGL backend init logging, across multiple separate
runs during this session's investigation. Screenshots at several different
camera-rotation angles all show: the mouse cursor sprite rendering correctly;
1–2 on-table models visible at a time (consistent with this sample's
intentionally close camera distance, `CameraDefaultDistance = 4.3`, and 45°
FOV — objects fill a large fraction of the screen by design, not a bug); the
flat-white-saturation finding and, once, the near-plane-clipping thin-line
artifact (both described above, both pre-existing CNA framework gaps). F1
help overlay confirmed via a temporary debug auto-trigger (`helpTimer_ = 10.0f`
forced in `LoadContent()`, removed before commit) — renders centered, on top
of everything, with the correct Windows-column control text. Not exercised via
real synthetic mouse/keyboard input this session (`xdotool` reliability
caveat, `NEXT.md` section 5): live cursor-drag-over-model name-label display,
and Escape/gamepad-Back exit (both simple, unconditional code paths ported
directly from the original, not expected to behave differently from any other
sample in this repo that already uses the identical pattern).
