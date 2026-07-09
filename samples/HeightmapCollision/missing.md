# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-10.** Builds with 0 warnings. Ported using CNA's stock
`Model`/`BasicEffect` for the sphere, and a hand-written runtime terrain-mesh
generator (`Terrain.hpp`, NOXNA) for the heightmap-derived ground — no CNA-side API
gaps blocked the sample's own game logic (movement, camera-follow, height
queries). Confirmed live for 7+ seconds with no crash across two separate runs;
screenshots show a fully textured, correctly lit terrain with the sphere sitting on
its surface (not floating/sinking), confirming the mesh and the collision height
data stay consistent with each other. (The historical "blocker" write-up below this
line described a real gap at the time it was written, 2026-07-06 — CNA's lit
`VertexPositionNormalTexture` path was resolved shortly after and DEFERRED.md item
#5 already reflected that; this port confirms it end-to-end.)

Source: `/rv/tmp/XNAGameStudio/Samples/HeightmapCollisionSample_4_0/HeightmapCollision/
HeightmapCollision/{HeightmapCollision.cs, HeightMapInfo.cs}` and
`HeightmapCollisionPipeline/{TerrainProcessor.cs, HeightMapInfoContent.cs}`.

## Terrain built at runtime instead of via a custom content-pipeline `ContentProcessor` (pre-existing gap, same adaptation already established by GeneratedGeometry)
**XNA behaviour:** `HeightmapCollisionPipeline.TerrainProcessor` (a
`[ContentProcessor]`-attributed `ContentProcessor<Texture2DContent, ModelContent>`)
runs at content-**build** time: it reads `terrain.bmp` as a
`PixelBitmapContent<float>`, builds a grid mesh via `MeshBuilder` (one vertex per
heightfield pixel, `position.Y = (pixel - 1) * terrainBumpiness`), chains to the
stock `ModelProcessor` (which fills in per-vertex normals it never explicitly
computed), attaches a `rocks.bmp`-textured `BasicMaterialContent`, and finally
attaches a `HeightMapInfoContent` (see below) to the built model's `Tag` property.
`HeightmapCollision.cs`'s own runtime code never runs any of this — it simply calls
`Content.Load<Model>("terrain")` and reads `terrain.Tag as HeightMapInfo`.
**CNA port behaviour:** CNA has no build-time, pluggable `ContentProcessor`
extensibility (DEFERRED.md item #18) and no `Model.Tag` equivalent, so the
build-time half of the original isn't directly portable. `Terrain.hpp` (NOXNA)
instead builds the same mesh **and** the `HeightMapInfo` collision data together at
runtime, in `LoadContent()`, replicating `TerrainProcessor.Process()`'s exact
algorithm (`terrainScale=30`, `terrainBumpiness=640`, `texCoordScale=0.1` —
`TerrainProcessor.cs`'s own default property values) and computing a smooth
per-vertex normal from the heightfield's gradient (central differences) in place of
`ModelProcessor`'s automatic normal generation. This is the **same adaptation this
repo's own `GeneratedGeometry` sample already established** for a structurally
identical `TerrainProcessor` (confirmed by reading `samples/GeneratedGeometry/
missing.md` and `samples/GeneratedGeometry/src/Terrain.hpp` directly before
choosing this approach) — not a new pattern invented for this sample.
**Root cause:** no build-time custom-`ContentProcessor` extensibility (DEFERRED.md
item #18).
**Tracked in:** DEFERRED.md item #18 (pre-existing).

## `HeightMapInfo`/`Model.Tag`: collision height data constructed directly instead of read back off a loaded `Model`
**XNA behaviour:** `HeightmapCollision.cs`'s `LoadContent()` does
`heightMapInfo = terrain.Tag as HeightMapInfo;` — the `HeightMapInfo` instance was
built once, offline, by `TerrainProcessor`/`HeightMapInfoContent`/
`HeightMapInfoWriter` at content-build time, serialized into the compiled `.xnb`,
and deserialized back by `HeightMapInfoReader` when `Content.Load<Model>("terrain")`
runs.
**CNA port behaviour:** `Terrain::Load()` builds the `HeightMapInfo` instance
directly, in the same function that builds the mesh, from the same computed
`heights` array (both use the identical formula,
`(heightfieldPixel - 1) * terrainBumpiness`, computed once and shared) — see
`Terrain::GetHeightMapInfo()`. The `HeightMapInfo` class itself
(`HeightMapInfo.hpp`) is otherwise an essentially unmodified, close port of
`HeightMapInfo.cs`'s own runtime game logic (`IsOnHeightmap`/`GetHeight`'s bilinear
interpolation) — only the *how it gets constructed* differs, not its own algorithm.
**Root cause:** no `Model.Tag` equivalent in CNA (same DEFERRED.md item #18 as
above — this is really one gap with two consequences, not two separate gaps).
**Tracked in:** DEFERRED.md item #18 (pre-existing).

## New nuance found: CNA's `.model.json` `ModelTypeReader` hardcodes 16-bit indices, even though `IndexBuffer` itself supports 32-bit (addendum to DEFERRED.md item #6)
**XNA behaviour:** `terrain.bmp` is 257×257, so `TerrainProcessor`'s generated mesh
has 257×257 = 66049 vertices — over the 65535 limit of a 16-bit index buffer. Real
XNA's stock `ModelProcessor` automatically selects `IndexElementSize.ThirtyTwoBits`
for a mesh this large; this is invisible to the sample's own code.
**CNA port behaviour:** confirmed via direct source read of `ModelTypeReader::Read()`
(`ContentManager.cpp`) that it unconditionally reads every mesh's index buffer as
`std::uint16_t` (`idxBytes.size() / sizeof(std::uint16_t)`, `IndexBuffer::SetData
(const std::uint16_t*, ...)`) — there is no code path to select 32-bit indices for a
`.model.json` mesh, regardless of vertex count. This is a genuine, narrower gap than
item #6's existing notes (which are about the *texture* field, not index width): a
future sample needing to `Content.Load<Model>()` any single mesh with more than
65535 vertices would silently get index/vertex-count truncation or an incorrect
draw, not a clean error. **Confirmed this is not a hard blocker for CNA in general**
— `IndexBuffer`/`IIndexBufferBackend` themselves fully support
`IndexElementSize::ThirtyTwoBits` end-to-end (confirmed both by header read and by
this port's own working 32-bit-indexed terrain draw, screenshot-verified below); the
gap is specifically in `ModelTypeReader`'s hardcoded assumption, not the underlying
buffer classes.
**Workaround applied:** sidestepped entirely by not routing the terrain through
`Content.Load<Model>`/`.model.json` at all — `Terrain::Load()` builds its
`VertexBuffer`/`IndexBuffer` directly, using the real
`IndexBuffer(device, IndexElementSize::ThirtyTwoBits, indexCount, BufferUsage::None)`
constructor (already a legitimate, tested path in CNA, independent of this gap).
**Root cause:** `ModelTypeReader::Read()`'s mesh-parsing loop only ever emits
`std::uint16_t` index data, with no branch on vertex count or an explicit
`"indexSize"` JSON field.
**Tracked in:** DEFERRED.md item #6 (new addendum, added this session — see there
for the exact file/line and suggested fix, mirroring the existing "no per-mesh
texture" addendum's shape).

## Terrain renders correctly textured and shaded (a positive side effect, unlike every other `Content.Load<Model>`-based sample so far)
**XNA behaviour:** the terrain renders with `rocks.bmp` tiled across its surface,
lit by `BasicEffect.EnableDefaultLighting()`.
**CNA port behaviour:** because the terrain in this port is **not** loaded via
`Content.Load<Model>`/`.model.json` (see above), it does not hit the "no per-mesh
texture field" gap that makes every other ported sample's model render as a flat,
fully-saturated white shape (DEFERRED.md item #6's addendum, first found in
PickingSample, repeated in TrianglePicking). `Terrain::Load()` builds its own
`BasicEffect` directly and calls `setTextureProperty(&rocksTexture_)`/
`setTextureEnabledProperty(true)` with a real, already-loaded `Texture2D` — this
works because `BasicEffect`/the EasyGL lit-textured shader path themselves are fine;
the gap is specifically in `.model.json`'s schema, which this terrain never goes
through. Confirmed live via screenshot: the terrain shows clear texture detail and a
visible shading gradient across its hills, not a flat white silhouette. The
**sphere**, which *is* loaded via `Content.Load<Model>("sphere")`, does still hit
the known gap and renders as a small flat white shape (see below) — this is an
expected, already-documented consequence, not a new bug, and this sample happens to
demonstrate both outcomes side by side in the same screenshot.
**Root cause:** N/A — positive finding; same underlying gap as item #6, just not
triggered for this particular mesh because of how it's loaded.
**Tracked in:** DEFERRED.md item #6 (existing).

## Sphere: flat, fully-saturated white shape with no shading gradient (already-tracked, expected)
**XNA behaviour:** `sphere.fbx` (converted from `pawball.tga`-textured source
geometry) renders with its ball texture, modulated by `BasicEffect`'s default
lighting.
**CNA port behaviour:** confirmed live via screenshot — the sphere renders as a
small, flat white blob (visible as an off-white ellipse on the terrain surface, its
apparent shape foreshortened by the camera's shallow viewing angle). This is the
same already-documented consequence as every other `Content.Load<Model>`-based
sample in this repo (DEFERRED.md item #6's addendum): `.model.json` has no
per-mesh texture field, so `BasicEffect.TextureEnabled` stays false and the default
white `DiffuseColor` combined with `EnableDefaultLighting()`'s bright 3-point rig
saturates to solid white for a broad range of surface normals. Not re-diagnosed —
recognized immediately as the known pattern from PickingSample/TrianglePicking.
`pawball.tga` was still converted to `pawball.png` and copied into `Content/` for
completeness (matching this repo's established precedent for unused textures), even
though nothing binds it.
**Root cause:** same as item #6's addendum — no per-mesh texture field in
`.model.json`.
**Tracked in:** DEFERRED.md item #6 (existing addendum; no new item needed).

## `sphere.fbx`'s `PreRotation` handled automatically (already-fixed tool behavior, not re-discovered as new)
**XNA behaviour:** N/A (content-pipeline detail invisible to the game's own code).
**CNA port behaviour:** `sphere.fbx`'s `Model::sphere01` node has a `PreRotation` of
`-90,0,0` (a Z-up → Y-up correction baked in by the exporting DCC tool) — the exact
same shape of node transform LensFlare's `terrain.fbx` had. `tools/
fbx_ascii2model.py` already applies baked `PreRotation`/`LclRotation`/
`LclScaling`/`LclTranslation` node transforms (fixed during LensFlare's port, see
DEFERRED.md item #6's existing tool-bug note), so this was handled correctly with
no extra work — confirmed via the tool's own console output
(`sphere01: applying baked node transform`) during conversion, and via the sphere
rendering right-side-up in every screenshot (not lying on its side, which is what an
unapplied `PreRotation` would have produced, per LensFlare's own prior finding).
**Root cause:** N/A — already fixed; not a new issue.
**Tracked in:** DEFERRED.md item #6 (existing).

## Specular color silently ignored (already-tracked, expected)
**XNA behaviour:** `TerrainProcessor.cs`'s `BasicMaterialContent.SpecularColor = new
Vector3(.4f, .4f, .4f)` adds a warm specular highlight to the terrain.
**CNA port behaviour:** `Terrain::Load()` still calls `setSpecularColorProperty
(Vector3(0.4f, 0.4f, 0.4f))` for source fidelity, but has no visible effect — the
EasyGL lit-textured GLSL shader has no Blinn-Phong specular term at all (confirmed
by `GeneratedGeometry/missing.md`, which found the identical gap for the same
constant on a structurally identical terrain).
**Root cause:** EasyGL lit shader implements diffuse + ambient + texture only, no
specular.
**Tracked in:** not in DEFERRED.md (per `GeneratedGeometry`'s own precedent — add if
specular ever becomes important to a sample).

## `GraphicsDevice::Clear(Color)` not clearing the depth buffer (pre-existing, latent, not observed to cause visible issues here)
**XNA behaviour:** `device.Clear(Color.Black)` clears color, depth, and stencil
together every frame.
**CNA port behaviour:** this port's `Draw()` uses the same single-argument
`Clear(Color)` overload, which (DEFERRED.md item #24, confirmed via direct source
read) never clears the depth buffer. Not observed to cause any visible artifact in
this sample's own screenshots (terrain and sphere both draw correctly, consistently,
across repeated frames) — likely because every pixel's depth is overwritten by the
terrain's own full-screen-covering geometry every frame regardless of stale prior
contents, but this wasn't specifically isolated/stress-tested.
**Root cause:** pre-existing CNA gap, not sample-specific.
**Tracked in:** DEFERRED.md item #24 (existing).

## `terrain.bmp`/`rocks.bmp` kept as raw `.bmp` (not converted to PNG), matching `GeneratedGeometry`'s precedent
**XNA behaviour:** N/A (content-pipeline detail).
**CNA port behaviour:** `CLAUDE.md`'s general asset-conversion guidance is "convert
texture source art to PNG," but `.bmp` is already a natively supported
`Content.Load<Texture2D>` extension in CNA (`Texture2DTypeReader::GetExtensions()`
includes `.bmp` directly, confirmed by source read) and this repo's own
`GeneratedGeometry` sample already established the precedent of keeping its
identically-purposed `terrain.bmp`/`rocks.bmp` as raw `.bmp` files rather than
converting them. Followed the same precedent here rather than introducing a
needless conversion step for an already-supported format. `pawball.tga` (the
sphere's unused source texture) *was* converted to PNG, matching the general
`.tga`-conversion precedent from PickingSample/TrianglePicking.
**Root cause:** N/A — deliberate, precedented choice, not a gap.
**Tracked in:** not planned.

## No `GameComponent`s in this sample — `DEFERRED.md` item #23 does not apply
**XNA behaviour:** `HeightmapCollisionGame` never creates or adds any
`DrawableGameComponent`/`GameComponent` (unlike PickingSample/Graphics3D/
TrianglePicking's `Cursor`, or Graphics3D's `Checkbox`es) — it draws the terrain and
sphere directly from `Draw()`.
**CNA port behaviour:** ported the same structure; no components, so
`Game::DoInitialize()`'s `ComponentAdded`-subscription-timing gap (DEFERRED.md item
#23) is simply not exercised by this sample at all.
**Root cause:** N/A.
**Tracked in:** not applicable.

## Near-plane-clipping-family bug: not observed in this sample
**Context:** `CameraShake`/`CustomModelClass`/`LensFlare`/`Graphics3D`/
`PickingSample`/`TrianglePicking` have all shown some form of this bug (a thin
diagonal line, or full invisibility at longer camera distances — `NEXT.md` section
4/8 task 2).
**This sample:** the camera sits `CameraPositionOffset = (0, 40, 150)` from the
sphere — a magnitude of ~155 units, noticeably closer than the ~1000+ unit
distances where the bug was previously observed. Confirmed live via two screenshots
taken several seconds apart: the sphere renders as a complete (if flat-white,
foreshortened) shape with no thin-line/dashed artifact, and the terrain — despite
spanning 7680×7680 units total — renders fully and continuously with no
clipping/disappearing geometry near the horizon. Not claiming the underlying bug is
fixed; just noting this sample's own camera geometry happens not to trigger it, so
there's nothing new to add to the existing tracking issue.
**Root cause:** N/A — negative finding.
**Tracked in:** `NEXT.md` section 4/8 task 2 (pre-existing; unaffected).

## F1 help overlay: standard 3-column table, no one-off variant needed
**XNA behaviour:** N/A (CNA-only addition per `CLAUDE.md`).
**CNA port behaviour:** `HeightmapCollision.htm`'s "Sample Controls" table extracts
cleanly with the stock `tools/gen_help_png.py` (no Windows-Phone-style extra leading
column, unlike PickingSample's `.htm`) — generated `Content/help.png` directly:
`Move the ball`: `UP ARROW, DOWN ARROW, LEFT ARROW, and RIGHT ARROW or W, A, S, and
D`; `Exit the sample`: `ESC or ALT+F4`. Not separately verified via a live keyboard
toggle this session (this repo's own `xdotool` keyboard-focus reliability caveat,
`NEXT.md` section 5) — implemented identically to every other sample's F1 overlay
code, which has been confirmed working elsewhere.
**Root cause:** N/A.
**Tracked in:** not planned.

## Verification
**Confirmed live:** built `HeightmapCollision_cna_samples` cleanly — 0 errors, 0
warnings (`cmake --build cmake-build-debug --target HeightmapCollision_cna_samples`).
Ran under `SDL_VIDEODRIVER=x11` — an 800×480 window opens
(`xwininfo -root -tree | grep -i heightmap` confirms it), and the process stayed
alive across two screenshots taken ~6 seconds apart with no crash and no error
output beyond normal EasyGL backend init logging. Screenshots show: a fully
textured, correctly lit, correctly shaded terrain (hills, texture detail, and a
visible shading gradient all present — the direct opposite of the flat-white
finding seen elsewhere in this repo); the sphere rendering as a small flat-white
blob sitting *on* the terrain surface at the expected position (not floating above
or sinking through it, confirming the mesh and the `HeightMapInfo` height data
built from the same source array stay mutually consistent); no near-plane-clipping
thin-line/invisibility artifact on either the terrain or the sphere at this
sample's own (relatively close, ~155-unit) camera distance. Movement/turning input
(arrow keys/WASD, gamepad D-pad/left stick) was not separately exercised via
synthetic input this session (the established `xdotool` keyboard-focus caveat,
`NEXT.md` section 5) — it is a direct, unmodified port of the original's own
`HandleInput()`/`UpdateCamera()` logic, using the same `Keyboard::GetState()`/
`GamePad::GetState()` APIs already proven working in every other ported sample.
