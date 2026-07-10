# Missing / Differences from XNA 4.0 original

This file was fully rewritten (2026-07-10) after actually porting the sample. The
previous version documented a genuine blocker (DEFERRED.md item #14, no
`Content.Load<TextureCube>`) that turned out to be avoidable via the same
construct-the-real-C++-object-directly bypass technique ReachGraphicsDemo's `EnvmapDemo`
(#005) proved for its own cubemap a few hours earlier this session — see that write-up
for the shared philosophy.

Source: `/rv/tmp/XNAGameStudio/Samples/RimLighting_4_0/RimLighting/RimLighting/Game1.cs`
(+ `Camera/Arcball.cs`, `Camera/ModelViewerCamera.cs`, `UI/{UIElement,Button,Slidebar}.cs`).

## TextureCube bypass: `OutputCube.dds` -- DEFERRED.md item #14, not fixed in `cna`

**XNA behaviour:** `Content.Load<TextureCube>("OutputCube")` loads a real, pre-baked
6-face DDS cubemap built at content-build time from `OutputCube.dds`.

**CNA port behaviour:** `ContentManager.cpp` still has no `TextureCubeTypeReader`
registered (confirmed live via a fresh read of `cna`'s current source, not assumed
stale) -- `Content.Load<TextureCube>` would still throw. Per this task's own explicit
constraint, this was **not** worked around by adding one to `cna` (that decision belongs
to the user). Instead:

1. `OutputCube.dds` was confirmed via `identify` (ImageMagick) to be a **real,
   uncompressed** (`xRGB8888`, not DXT-compressed) 6-face DDS cubemap, 64x64 per face --
   simpler than `EnvmapDemo`'s own cubemap (that one had to be *procedurally generated*
   at conversion time from a single flat photo; this one already has 6 real baked
   faces). `TextureCube::DDSFromStreamEXT` (CNA's own NOXNA direct-DDS-stream loader)
   was checked first and found **unusable** here: its source (`TextureCube.cpp`) only
   decodes DXT1/DXT3/DXT5-compressed cube maps, throwing `NotSupportedException` for any
   other pixel format -- `OutputCube.dds`'s uncompressed format doesn't qualify.
2. Each of the 6 faces was extracted directly with ImageMagick:
   `convert OutputCube.dds[N] -alpha off -define png:color-type=2 envmap_<face>.png`
   for `N` = 0..5, producing 6 ordinary opaque RGB PNGs (`envmap_posx.png` ..
   `envmap_negz.png`).
3. These are loaded via the normal, proven `Content.Load<Texture2D>` path and their
   pixel data copied directly into a real CNA `TextureCube` via its own
   `SetData(CubeMapFace, const Color*, int)` API (`RimLightingGame::LoadCubemap()`) --
   bypassing `ContentManager`/`TextureCubeTypeReader` entirely, the same bypass
   philosophy as `HeadModel.hpp` (below), applied to `TextureCube` instead of `Model`.

**Face-index-to-`CubeMapFace` mapping: confirmed correct on the first try, no swap
needed.** `TextureCube::DDSFromStreamEXT`'s own DXT-decoding loop
(`for (int face = 0; face < 6; ++face) { ... result.SetData(static_cast<CubeMapFace>(face), ...) }`)
directly `static_cast`s a DDS face index 0..5 to `CubeMapFace(face)` -- i.e. CNA's own
DDS-loading code already assumes DDS's on-disk face order is
`PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ`, exactly matching
`CubeMapFace`'s declared enum order. `envmap_{posx,negx,posy,negy,posz,negz}.png`
(ImageMagick indices `[0]`..`[5]`) were mapped 1:1 to that same enum order. **Confirmed
visually, not just assumed:** `identify`-ing the 6 extracted faces before ever writing
any C++ showed 5 faces solid black and exactly 1 (`envmap_negz.png`, DDS index 5) a
bright solid orange -- matching `RimLighting.htm`'s own description of the cubemap's
construction exactly ("all faces, except the back face, are dark... a bright color... on
the back face"). Once rendering, the resulting rim highlight appears exactly where a
real rim light should (see "Verification" below) -- a wrong face mapping would have
produced the bright highlight on an unrelated/wrong-shaped region of the silhouette, or
made the whole model uniformly bright/dark instead of a clean rim. No swap was needed.

**Tracked in:** DEFERRED.md item #14 (still open; RimLighting no longer blocks on it).

## `head.fbx` bypass -- DEFERRED.md item #26, plus a real non-identity parent-bone transform

**XNA behaviour:** `Content.Load<Model>("head")` loads a single-mesh model whose mesh
node ("`Model::pasted__polySurface14`") is parented under a Null "`Model::group`" node
carrying its own real `Lcl Translation`/`Lcl Rotation` (confirmed by direct read of
`head.fbx`: translation ~`(0, -14.48, 0.16)`, rotation `(0, 180, 0)` degrees -- the mesh
node's *own* local transform is identity). `Game1.cs`'s
`effect.World = transforms[mesh.ParentBone.Index] * world` applies this parent-bone
transform to the mesh every frame via `Model.CopyAbsoluteBoneTransformsTo()`.

**CNA port behaviour:** Per this session's own established DEFERRED.md item #26 (every
stride-32 `.model.json` in this repo hits a real `ModelTypeReader` vertex-stride/vtable
mismatch, confirmed on 5+ assets across 5 samples), this went straight to the same
`RawModel`-style bypass (`HeadModel.hpp`) rather than trying `Content.Load<Model>` first.
Since the bypass reads raw, un-baked mesh-local vertex data with no `ModelBone`/
`ParentBone` concept at all, the parent "group" node's real transform had to be baked
into the vertex data at **conversion time** instead. A one-off Python script
(`rimlighting_head_convert.py`, not committed to `tools/` -- kept in the session's own
scratch directory, described here for reproducibility) reused
`tools/fbx_ascii2model.py`'s own parsing helpers (`parse_model_transform`,
`transform_position`/`transform_normal`, `triangulate`, `build_buffers`) but pointed
`parse_model_transform` at the **parent "group" Null block** instead of assuming the
mesh's own (identity) local transform was the whole story, then applied both (mesh-local
then group, matching XNA's own child-local-times-parent-absolute chain) to every
position/normal before triangulating. Since the group node has no `AnimationCurve`/Take
referencing it (confirmed via grep -- a static, constant transform), baking it in at
conversion time is mathematically identical to what `CopyAbsoluteBoneTransformsTo()`
would recompute every single frame for this non-animated 2-node hierarchy. Output:
`head.model.json` (documentation only, matching this repo's convention of keeping one
alongside the `_verts.bin`/`_idx.bin` even though the runtime bypasses it) +
`head_pasted__polySurface14_verts.bin`/`_idx.bin` (32752 vertices, 16376 triangles),
read directly by `HeadModel.hpp` and bound to a real `EnvironmentMapEffect` the same way
`ChaseCamera`'s `RawModel.hpp` binds a `BasicEffect`.

**Tracked in:** DEFERRED.md item #26 (bypass applied proactively, matching this
session's own established precedent -- not re-confirmed against a plain
`Content.Load<Model>` build first, since 5+ prior confirmations already exist).

## New tooling bug found and fixed: `tools/fbx_ascii2model.py` assumed `LayerElementNormal` is always "ByPolygonVertex" -- it usually isn't

**Found while porting RimLighting.** A first pass of the conversion above (before this
fix) rendered a recognizable head silhouette, but with severe, localized jagged/scrambled
dark patches wherever the environment-map reflection was active (confirmed via a
temporary debug build forcing `EnvironmentMapAmount` to `0`, which -- since
`DirectionalLight0`'s own `DiffuseColor` is never set anywhere in `Game1.cs`, matching
its CNA default of black -- rendered a perfectly clean, artifact-free silhouette,
isolating the problem specifically to the reflection/normal-dependent term, not the raw
geometry).

**Root cause, confirmed by direct source read:** `head.fbx`'s own `LayerElementNormal`
block declares `MappingInformationType: "ByVertice"` -- meaning there is **exactly one
normal per unique vertex/control-point** (same array length as `Vertices:`, confirmed
live: both exactly 8213 entries), indexed the same way `positions` is (`pos_idx`), NOT
per-polygon-corner like UVs are. `tools/fbx_ascii2model.py`'s `build_buffers()`
unconditionally indexed the `Normals:` array by `flat_idx` (the per-polygon-corner flat
index, 0..32751 for this mesh) regardless of this field -- correct only for
`"ByPolygonVertex"`-mapped files. For a `"ByVertice"`-mapped file with more polygon
corners than unique vertices (the overwhelmingly common case for any mesh with vertex
sharing), this reads normals from essentially unrelated array slots for the ~8213
corners where `flat_idx` happens to stay in bounds, and silently falls back to a
hardcoded "straight up" `(0,1,0)` default normal for **every other corner** (here, about
75% of them) -- exactly the kind of localized, patchy corruption a binary
all-black-except-one-bright-face cubemap reflection would make starkly, jaggedly
visible (a wrong texture on a diffuse-lit surface just looks "a bit off"; a wrong normal
under a high-contrast reflection can look like scrambled static).

**A repo-wide grep across every FBX file used by this session's already-shipped samples
found `"ByVertice"` is actually the *more common* of the two mapping modes, not
`"ByPolygonVertex"`:** `Ship.fbx` (ChaseCamera, converted directly via
`tools/fbx_ascii2model.py`), `saucer.fbx`/`model.fbx` (ReachGraphicsDemo, same tool) are
all `"ByVertice"` too -- meaning this bug likely also affects their own already-shipped
`_verts.bin` normals (not re-verified or re-shipped here -- out of scope for a
single-sample porting session; see DEFERRED.md's new item for the exact follow-up).
Only `tank.fbx` (ReachGraphicsDemo, converted by its own separate one-off script, not
this shared tool), `P2Wedge.FBX`/`Cats.FBX` (PickingSample/TrianglePicking), and
`maze1.FBX`/`marble.FBX` (MarbleMaze, binary FBX -- converted via a different pipeline
entirely) are confirmed `"ByPolygonVertex"` or not applicable.

**Fix:** `tools/fbx_ascii2model.py`'s `parse_mesh_block()` now also returns the
`LayerElementNormal`'s own `MappingInformationType`; `build_buffers()` takes it as a new
(default-backward-compatible) parameter and indexes `Normals:` by `pos_idx` when it's
`"ByVertice"`/`"ByControlPoint"`, by `flat_idx` (the old, still-correct-for-that-case
behavior) otherwise. **Confirmed this does not change any already-shipped
`"ByPolygonVertex"` asset:** re-ran `P2Wedge.FBX` and `Cats.FBX` (PickingSample) through
the fixed tool and diff'd the output `_verts.bin`/`_idx.bin` byte-identical to the
already-shipped files.

**Tracked in:** DEFERRED.md item #30 (new).

## `Head_Diff.tga` not converted -- confirmed unused at runtime

**XNA behaviour:** `head.fbx`'s own material (`Material::pasted__Head2`) references
`Head_Diff.tga` as its diffuse texture, and `RimLightingContent.contentproj` builds
`head.fbx` with `<ProcessorParameters_DefaultEffect>EnvironmentMapEffect</...>` (the
stock `ModelProcessor`'s own "assign this stock effect to every mesh part" option --
explaining why `Game1.cs`'s `foreach (EnvironmentMapEffect effect in mesh.Effects)`
works with no custom `.fx` file anywhere in this sample). However, `Game1.cs` **always**
overwrites `effect.Texture = texure2D` (the separately-loaded, all-white `blankTex`) on
every mesh's effect, every frame -- `Head_Diff.tga` is therefore never actually sampled
at runtime, confirmed via a full-file grep of `Game1.cs` (zero references to
`Head_Diff`/the FBX material name anywhere).

**CNA port behaviour:** `Head_Diff.tga` was not converted or shipped at all (it is a
4x4-pixel placeholder file in the original content anyway, per `identify`) --
`HeadModel.hpp`/`RimLightingGame.hpp` never bind it, matching the real runtime behavior
exactly, not a gap.

**Tracked in:** not planned (not a difference in actual rendered output).

## NOXNA input substitution: mouse-synthesized `TouchLocation` instead of real touch

**XNA behaviour:** `Update()` reads `TouchPanel.GetState()` every frame and feeds each
active touch point to the UI elements (`Button`/`Slidebar`) and
`ModelViewerCamera`'s two arcballs via `HandleTouch(TouchLocation)`.

**CNA port behaviour:** Confirmed via a fresh read of `cna`'s current
`Input/Touch/TouchPanel.cpp` that `GetState()` only ever reports real touch hardware
(`InputManager::GetTouchState()`, itself fed only by real SDL touch-finger events) --
no mouse-to-touch synthesis fallback exists, unlike some other engines' desktop touch
emulation. This dev machine has no touchscreen. `RimLightingGame::SynthesizeTouches()`
builds at most one `TouchLocation` per frame from `Mouse::GetState()`'s left-button edge
transitions (`Pressed` on the down edge, `Moved` while held, `Released` on the up edge,
nothing at all while idle -- matching a real empty `TouchCollection`, not a fabricated
"always-present" touch point), with a fixed nonzero synthetic id (`1`) fed through the
**exact same, unmodified** `Arcball`/`Button`/`Slidebar`/`ModelViewerCamera`
`HandleTouch(TouchLocation)` methods the C# original uses -- the touch abstraction
itself is preserved faithfully; only its input source changes.

**Nonzero synthetic touch id, not `0`:** `Button.cs`'s own `HandleTouch()` uses
`pressId == 0` as its "not currently pressed" sentinel, then sets
`pressId = loc.Id` on press -- meaning touch id `0` would be indistinguishable from "no
press" on *any* platform whose first touch happens to be assigned id 0 (a latent
ambiguity in the original itself, not introduced by this port). Since this port fully
controls the synthetic id, it deliberately uses `1`, sidestepping the ambiguity
entirely rather than reproducing it.

**Tracked in:** not planned (matches this repo's own established, repeatedly-applied
touch-to-mouse substitution pattern -- see NEXT.md section 6's input-fallback table).

## NOXNA: `Draw()` split into a 3D pass then a single `SpriteBatch` `Begin()`/`End()` block

**XNA behaviour:** `Button.cs`'s own `Draw(SpriteBatch)` mixes a 3D `BasicEffect`-drawn
box with its own `SpriteBatch.Begin()`/`DrawString()`/`End()` block in the same method
(its own doc comment warns "should not be called from within a SpriteBatch.Begin/End
block" -- i.e. it must be called *outside* any other such block, but opens its own).
`Slidebar.cs`'s `Draw(SpriteBatch)` does the same (rect + text, its own
`Begin()`/`End()`). `Game1.cs`'s own `Draw()` calls each of the 3 `uiElementList` items'
`.Draw(spriteBatch)` once per frame -- 3 separate `Begin()`/`End()` blocks total, after
the 3D model draw.

**CNA port behaviour:** Following CLAUDE.md's own established F1-help-overlay
convention (a single `SpriteBatch` `Begin()`/`End()` block per frame, drawn last), each
UI element's rendering is split into a 3D part (`Button::DrawBox()`, called once, before
any `SpriteBatch` `Begin()`) and a 2D part (`Button::DrawText(SpriteBatch&)`/
`Slidebar::DrawText(SpriteBatch&)`, assuming an already-open `Begin()` block) instead of
each managing its own `Begin()`/`End()`. `RimLightingGame::Draw()` opens exactly one
`SpriteBatch` block covering the button's text, both sliders, and the F1 help overlay
(drawn last, on top).

**Root cause:** desktop-porting-session convention (this repo's own CLAUDE.md), not
required to avoid any *specific* confirmed CNA bug in this sample (unlike
DEFERRED.md item #28, which does not apply here since this sample's own 3D content is
drawn before any 2D/`SpriteBatch` content every frame, the opposite order from what item
#28 requires to trigger).

**Tracked in:** not planned (style/consolidation choice, not a functional gap).

## Desktop conventions: windowed (not fullscreen), Escape added, dead fields/dead code omitted

- `graphics.IsFullScreen = true` -> kept windowed (`setIsFullScreenProperty(false)`),
  matching every other Windows-Phone-style sample in this repo (AccelerometerSample,
  TiltPerspective, Orientation, ...). Not planned (desktop dev-loop practicality).
- `GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed` is ported
  unchanged; `Keys::Escape` is also checked (a CNA/desktop addition, matching every
  other phone-sample port in this repo). Not planned.
- `TargetElapsedTime = TimeSpan.FromTicks(333333)` (30 fps) is kept faithfully.
- `matrixWorld`, `matrixView`, `vec2RotWorld`, `vec2RotCamera` (`Game1.cs` fields) and
  `UIElement.WordWrap()` are all confirmed-dead code in the original (declared,
  initialized, but never read or called anywhere else in the file -- confirmed via a
  full-repo grep of `RimLighting_4_0`) and are not ported. No behavior difference.

**Tracked in:** not planned.

## Content asset summary

| Asset | Source | Conversion |
|---|---|---|
| `Font.font.json`/`Font.png` | `Font.spritefont` (`Segoe UI Mono`, 12px) | `tools/make_font.py` (DejaVuSansMono.ttf, substituting the original's Segoe UI Mono, at the same declared 12px size) |
| `blankTex.png` | same | copied verbatim (already a 4x4 opaque-white PNG) |
| `head.model.json` + `head_pasted__polySurface14_{verts,idx}.bin` | `head.fbx` | one-off script (`rimlighting_head_convert.py`, reusing `tools/fbx_ascii2model.py`'s helpers) baking the parent "group" Null node's real transform in, plus the fixed tool's now-correct `"ByVertice"` normal handling -- see above |
| `envmap_{posx,negx,posy,negy,posz,negz}.png` | `OutputCube.dds` | `convert OutputCube.dds[N] -alpha off -define png:color-type=2 ...png`, N=0..5 (ImageMagick), then copied into a real `TextureCube` via `SetData()` at runtime -- see above |
| `help.png` | this sample's own `.htm` has no "Sample Controls" table (prose description only) | one-off script using `tools/gen_help_png.py`'s own `render_png()` helper with hand-written control text |

`Head_Diff.tga` was not converted (confirmed unused at runtime -- see above).

## Verification

Built clean (0 warnings) via
`cmake --build cmake-build-debug --target RimLighting_cna_samples`. Ran under
`SDL_VIDEODRIVER=x11`, confirmed no crash over several runs totaling 15+ seconds, each
screenshotted (`import -window <id>`) via the window's own confirmed PID (matching this
repo's own established `xdotool getwindowpid` verification step). Screenshots confirm:

- A recognizable head silhouette, mostly dark/unlit (`DirectionalLight0`'s
  `DiffuseColor` is never set anywhere in `Game1.cs`, defaulting to black -- matching
  the original exactly), with a clean, continuous, orange **rim-light highlight**
  tracing the silhouette's edges (ears, jaw, chin, nose bridge, mouth outline) --
  exactly the effect `RimLighting.htm`'s own "How the Sample Works" section describes,
  and a clear, unambiguous confirmation that both the `TextureCube` face conversion
  and the face-order mapping are correct (a wrong mapping would have produced a bright
  highlight in the wrong place/shape, or no discernible rim at all, not this clean,
  anatomically-consistent result).
- Before the `tools/fbx_ascii2model.py` normal-mapping fix (above), the same view showed
  a recognizable but badly artifacted silhouette (jagged black/bright scrambling
  wherever the reflection was active) -- isolated via a temporary debug build
  (`EnvironmentMapAmount` forced to `0`, then to a moderate `1.0`/`1.0`) to be a normal
  data bug, not a face-mapping or geometry bug, before the fix. Both temporary debug
  overrides were reverted before the final build (confirmed via a clean from-scratch
  rebuild afterward, 0 warnings).
- The UI (the "Rotating World" button, both sliders showing their live `Amount: 2.5`/
  `Thickness (FresnelFactor): 6` text) renders correctly, matching the default slider
  values `Game1.cs` sets.
- The F1 help overlay was confirmed via this repo's own established temporary-forced-
  timer pattern (`helpTimer_` initialized to `10.0f` instead of `0.0f`, screenshotted,
  then reverted -- confirmed via a clean rebuild afterward) rather than a live keypress,
  and rendered correctly (semi-transparent panel, correct control list, centered).

**Not exercised live this session:** interactive mouse-drag rotation (arcball) and the
"toggle world/camera rotation" button click. Per this repo's own documented
`xdotool`-on-a-shared-desktop caveat (a real, unrelated window was holding actual X
focus during this session, confirmed via `xdotool getactivewindow` before attempting
any synthetic input), sending synthetic mouse clicks/drags risked misdirecting input to
someone else's session -- skipped rather than risk it, matching this repo's own
established caution (e.g. ReachGraphicsDemo's own `missing.md` records the identical
decision). The arcball/button code itself is a near-verbatim, unmodified port of
`Arcball.cs`/`ModelViewerCamera.cs`/`Button.cs` (see each file's own header comment) with
only the touch-vs-mouse input *source* changed -- reviewed line-by-line against the
originals rather than exercised interactively.
