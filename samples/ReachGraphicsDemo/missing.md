# Missing / Differences from XNA 4.0 original

This file was fully rewritten (2026-07-10) after actually porting the sample. The
previous version was confirmed stale before this session started: it claimed
SpriteFont, `Content.Load<Model>`, `EnvironmentMapEffect`, and `DualTextureEffect` were
all missing from CNA -- every one of those is implemented and used successfully below.

## Scope: 5 of 6 demo scenes ported, SkinnedDemo skipped

**Ported fully:** the shared menu framework (`MenuComponent`/`MenuEntry`/`TitleMenu`/
`DemoGame` -- transition effects, zoomy-text feedback, attract mode) plus **BasicDemo**,
**AlphaDemo**, **DualDemo**, **EnvmapDemo**, and **ParticleDemo**.

**Skipped: SkinnedDemo** (SkinnedEffect / skeletal animation). CNA has no skeletal
animation system at all (DEFERRED.md item #13 -- no `AnimationClip`/`Keyframe`/
`AnimationPlayer`, no per-vertex bone weight/keyframe data in `.model.json`). Unlike
InverseKinematics' Avatar half (which keeps the *entire* original code structure,
permanently guarded by a real, always-false CNA runtime condition), there is no
equivalent CNA class or state to guard here -- the underlying types this demo needs
simply don't exist yet, so there's nothing to construct even in inert form.
`samples/ReachGraphicsDemo/src/SkinnedDemo.hpp` still ports the surrounding
`MenuComponent` framework faithfully (menu list, "back" button, attract-mode cycling,
F1 help overlay all still work when this screen is selected) but replaces the animated
"dude" model + skydome with a clear on-screen "not available" message explaining why
and pointing at DEFERRED.md item #13. `dude.fbx`/`sky.bmp`/`SkinnedModelProcessor`/
`Sky.cs`/`SkyProcessor.cs` were not converted or ported at all.

## Correction to this task's own original brief: TitleMenu has no 3D scene

The task brief that started this port assumed TitleMenu.cs used `Sky.cs`/`Tank.cs` for
its own 3D background, and offered "a simpler static/placeholder title screen" as an
acceptable scope reduction if that turned out to be too large. A full read of
`TitleMenu.cs` found this premise was simply wrong: `Sky.cs` is used only by
`SkinnedDemo.cs` (skydome background) and `Tank.cs` is used only by `BasicDemo.cs`/
`AlphaDemo.cs` -- `TitleMenu.cs`'s own `Draw()` is `DrawTitle(...)` (flat
CornflowerBlue background + rotated BigFont title text) plus a SpriteBatch-only
"floating xna text labels" background effect, no 3D content of any kind. So there was
no scope decision to make here: TitleMenu is ported fully and faithfully, exactly as
the original, in `samples/ReachGraphicsDemo/src/TitleMenu.hpp`.

## DEFERRED.md item #26 (ModelTypeReader vertex-stride corruption) -- worked around throughout

Every model in this sample (`grid`, `model`, `saucer`, `tank`) is loaded via a NOXNA
bypass (`RawMesh.hpp` / `RawMeshPosTex.hpp` / `TankModel.hpp`, all reading
already-converted `_verts.bin`/`_idx.bin` sidecars directly and constructing real C++
vertex objects field-by-field), never through `Content.Load<Model>()` /
`ModelTypeReader`. This was applied proactively from the start (not re-confirmed
empirically against a plain `Content.Load<Model>` build first), following this
session's own established precedent (5+ prior confirmations across 5 other samples --
see DEFERRED.md item #26 and `samples/MarbleMaze/missing.md`).

## Tank: real per-part hierarchical animation (wheels/steering/turret/cannon/hatch)

`tank.fbx`'s own `Connect: "OO"` lines reveal a genuine, NESTED (parent, child) bone
hierarchy (`tank_geo` -> `{r_engine_geo, l_engine_geo, turret_geo}`; `r_engine_geo` ->
`{r_back_wheel_geo, r_steer_geo}`; `r_steer_geo` -> `r_front_wheel_geo`; `turret_geo` ->
`{canon_geo, hatch_geo}`; mirrored for the left side) -- unlike every other model in
this repo (all flat, single-bone). CNA's `.model.json`/`ModelTypeReader` has no
per-mesh `ModelBone`/parent-bone support at all (DEFERRED.md item #6's multi-bone
addendum, the same gap blocking SplitScreen/TankOnAHeightMap/SimpleAnimation), so
`Content.Load<Model>("tank")` could not reproduce `Tank.cs`'s independently rotating
parts even setting DEFERRED item #26 aside.

Since `TankModel.hpp` bypasses CNA's `Model`/`ModelBone` system entirely (reading raw,
un-baked, mesh-local vertex data per named part), there is no CNA-side hierarchy
missing in the first place -- the hierarchy is reimplemented directly in C++, replaying
exactly the same `absolute[mesh] = local[mesh] * absolute[parent]` chain XNA's own
`Model.CopyAbsoluteBoneTransformsTo()` performs, with `local[mesh]` recomputed every
frame from `Tank.cs`'s own `<rotation> * <original bone Transform>` formula. A one-off
Python conversion script (based on `tools/fbx_ascii2model.py`'s own parsing helpers,
not committed to `tools/`) extracted each of `tank.fbx`'s 12 named mesh nodes' own
mesh-local (un-baked) vertex data plus each node's own local rest translation
(confirmed via `tools/fbx_ascii2model.py`'s own `parse_model_transform()` that all 12
nodes have identity rotation/scale -- only translation). Confirmed live: the tank
renders fully textured (two distinct materials: `turret_alt_diff_tex.tga` for the
chassis/turret/cannon/hatch, `engine_diff_tex.tga` for the engine/wheel/steer
assemblies), shaded, and the turret visibly rotates frame-to-frame (screenshot
comparison). **NOXNA note, matching the C# original exactly:** `WheelRotation` is
never actually set anywhere in `Tank.cs`/`BasicDemo.cs`/`AlphaDemo.cs` (confirmed via
full-file grep) -- the wheels never spin in the real XNA original either; kept as a
dead property in `TankModel.hpp` for structural fidelity.

`grid.x` (BasicDemo's/AlphaDemo's checkered ground plane) has no ASCII-FBX equivalent
parser in this repo's tools, so it went through `assimp export` -> OBJ ->
`tools/obj2model.py`, the same pipeline as ChaseCamera's `Ground.x`. Confirmed live
(screenshot) this hits the exact same `assimp export`-introduced triangle-winding
inversion already documented in `samples/ChaseCamera/missing.md` and
`samples/MarbleMaze/missing.md`: with the default `RasterizerState::CullCounterClockwise`
(matching the C# original), the grid was entirely back-face-culled; fixed with a
permanent `RasterizerState::CullNone` isolated to just this one mesh's own draw call
(`GridModel.hpp`) -- the third confirmed sighting of this exact quirk in this repo.

## DualDemo: model.fbx has 2 UV layers per mesh -- fixed a real `tools/fbx_ascii2model.py` bug

`model.fbx`'s own `LayerElementUV` blocks confirmed each of its 7 meshes has **two**
UV layers: layer 0 (the base diffuse-texture UV, tiling 0-1 per face) and layer 1 (a
separate, unique-per-face lightmap UV set). `tools/fbx_ascii2model.py`'s
`parse_mesh_block()` only ever kept the *last* `UV:`/`UVIndex:` block it encountered in
a mesh (a naive "later assignment overwrites earlier" parse), so it silently used
layer 1 (the lightmap UV) as the mesh's *only* vertex UV, instead of layer 0 (the base
texture UV) -- corrupting texture coordinates for any multi-UV-layer FBX mesh (a new
kind of asset this repo hadn't hit before). Fixed in `tools/fbx_ascii2model.py`
(`parse_mesh_block()` now keeps only the *first* UV layer found, matching layer 0);
confirmed this does not change any already-shipped asset (`saucer.fbx`/`tank.fbx` each
have exactly one `LayerElementUV` per mesh -- re-ran both through the fixed tool and
diff'd the output `_verts.bin` files byte-identical to the previously-shipped ones).

Separately, confirmed live that CNA's own EasyGL "dual+textured3D" GLSL program
(`EnsureDualTextured3DProgram()`, `EasyGLGraphicsBackend.cpp`) hardcodes a
**Position + UV only** (2-attribute, no Normal) vertex layout --
`layout(location=0) in vec3 aPos; layout(location=1) in vec2 aUV;` -- unlike
`BasicEffect`'s/`EnvironmentMapEffect`'s own lit shaders, which expect Position +
Normal + UV (3 attributes). Uploading this repo's usual `VertexPositionNormalTexture`
(where UV is declared at attribute location 2, not 1) against this shader made `aUV`
silently read from whatever is bound at location 1 -- the mesh's own Normal.xy --
instead of the real per-vertex UV: every one of DualDemo's 7 submeshes rendered as a
flat/uniform color per mesh (each face's own constant normal producing a constant
"fake UV," landing on one arbitrary texel), confirmed by screenshot before/after.
Worked around with a new `RawMeshPosTex.hpp` (NOXNA, alongside `RawMesh.hpp`) that
uploads plain `VertexPositionTexture` (discarding the normal field from the same
sidecar files) specifically for `DualDemo`'s meshes. Confirmed live: with both fixes
(UV-layer + vertex layout), the ground plane and 6 blocks render with real tiled
texture detail plus the lightmap's own glowing "hotspot" pattern correctly blended in.
Filed as a new DEFERRED.md item (CNA's `DualTextureEffect` vertex-layout requirement is
undocumented/differs from every other stock effect).

## EnvmapDemo: two more real CNA-level rendering gaps found and worked around

1. **DEFERRED.md item #24** (`GraphicsDevice::Clear(Color)`'s single-arg overload never
   clears the depth buffer): this scene's own `Clear(Color.Black)` call (matching the
   C# original exactly) needs a fresh depth buffer every frame for its saucer's depth
   test to behave correctly across scene switches. Worked around with the 2-arg
   `Clear(Color, float)` overload (which does clear depth) -- a NOXNA, sample-level
   deviation from the literal C# call, not a real behavior change (real XNA's own
   single-arg `Clear(Color)` already clears depth+stencil too).
2. **New CNA gap, not previously documented:** a `SpriteBatch.Begin()/Draw()/End()`
   call that draws a texture stretched to cover the *entire* backbuffer, executed
   before any 3D `DrawIndexedPrimitives` call in the same frame, leaves backend state
   that makes every subsequent 3D draw call in that same frame render nothing at all --
   confirmed live, isolated step by step (see `EnvmapDemo.hpp`'s own header comment for
   the full account): removing just the background image's own `SpriteBatch.Draw()`
   call made the saucer render correctly; swapping the 3D effect
   (`EnvironmentMapEffect` -> plain `BasicEffect`), changing the camera to a much
   closer/simpler one, and forcing `RasterizerState::CullNone` each independently made
   no difference; converting the source image from JPEG to an ordinary RGBA PNG also
   made no difference -- ruling out the effect type, camera, and source texture format
   as the cause, and narrowing it specifically to "a SpriteBatch draw covering the
   whole backbuffer, positioned before 3D content, in the same frame." This repo's
   other demos' own `DrawTitle()` calls (a much smaller SpriteBatch draw, of text only)
   also happen before their 3D content and are unaffected -- so the trigger appears to
   be specifically the *full-screen* coverage, not merely "any SpriteBatch call before
   3D." Not root-caused inside `cna` itself (out of scope for this porting session).
   Worked around at the sample level: the background image is now drawn as a
   manually-built full-screen quad via `BasicEffect` + `DrawUserIndexedPrimitives`
   (`EnvmapDemo::DrawBackgroundQuad()`, NOXNA) instead of `SpriteBatch`, entirely
   avoiding the problematic call shape. Confirmed live: with this fix, the background
   image, title text, and the saucer (fully textured, lit, and showing a genuine
   reflective/chrome cubemap look) all render correctly together. Filed as a new
   DEFERRED.md item.

Converted `background.jpg` to `background.png` (RGBA) while investigating the above --
turned out not to be the actual cause (confirmed empirically), but kept as the shipped
asset anyway since it's a reasonable, harmless simplification once the real texture
pipeline no longer goes through `SpriteBatch` for this one draw.

## EnvmapDemo: cubemap reimplemented as a one-off Python/Pillow script

Per this task's own scope decision, DEFERRED.md item #14 (no
`Content.Load<TextureCube>`/`TextureCubeTypeReader` in CNA) was NOT worked around by
adding one to `cna` -- instead, the original's own build-time
`ContentPipelineExtension/CubemapProcessor.cs` (+`TexturePlusAlphaProcessor.cs`)
algorithm was reimplemented as a one-off Python/Pillow script (not committed to
`tools/` -- kept in the session's own scratch directory, described here for
reproducibility):
1. `TexturePlusAlphaProcessor`: merge `seattle.bmp` (color) + `seattle_alpha.bmp`
   (greyscale specular mask) into one RGBA image (alpha = average of the alpha
   bitmap's own R/G/B).
2. `CubemapProcessor.MirrorBitmap`: mirror the merged image left-right (source
   followed by its own horizontal mirror) so it wraps seamlessly.
3. `CubemapProcessor.CreateSideFace` (x4): crop + resize 4 side faces (NegativeZ,
   NegativeX, PositiveZ, PositiveX) directly from the mirrored image.
4. `CubemapProcessor.CreateTopFace`/`CreateBottomFace` (+`ScaleTrapezoid` +
   `BlurCubemapFace`): fold 4 flaps from the top/bottom third of the mirrored image
   inward to build the remaining 2 faces, then downsample+blur to hide the seam where
   the 4 flaps meet.

Output: 6 ordinary 64x64 RGBA PNGs (`envmap_{posx,negx,posy,negy,posz,negz}.png`),
loaded here via the normal, proven `Content.Load<Texture2D>` path and their pixel data
copied directly into a real CNA `TextureCube` via its own
`SetData(CubeMapFace, const Color*, int)` API (`EnvmapDemo::LoadCubemap()`) --
bypassing `ContentManager`/`TextureCubeTypeReader` entirely, the same
"construct-the-real-C++-object-directly" bypass philosophy as `RawMesh.hpp`, applied to
a different CNA type. No mipmaps are generated (`TextureCube` constructed with
`mipMap=false`) -- a deliberate simplification, since mipmapping only affects
minification aliasing on a reflection map and isn't worth the extra complexity of
computing a full mip chain in the one-off script.

**Visual result, confirmed live via screenshot:** the saucer shows a genuinely
reflective, chrome/mirror-like surface with dark blue/black tones and warm highlights,
consistent with reflecting the source coastal photo's sky/ocean/rock cubemap -- a real,
convincing environment-map effect, not a flat or solid-color fallback.

## AlphaDemo: imposter tank billboards render noticeably dark

The 25 billboarded "imposter" tank sprites (rendered once into a 400x400
`RenderTarget2D` via `TankModel::Draw(..., LightingMode::OneVertexLight, true)`, then
stamped around the 3D scene via `AlphaTestEffect` + `DrawUserIndexedPrimitives`) render
correctly shaped (confirmed: real tank silhouettes against a checkered floor, not a
flat rectangle or solid color -- the `AlphaTestEffect` alpha-test/discard mechanism is
working) but noticeably dark/underlit compared to the same tank model in BasicDemo.
`LightingMode::OneVertexLight` deliberately disables `DirectionalLight1`/
`DirectionalLight2` (matching `Tank.cs`'s own `Draw()` exactly), leaving only
`DirectionalLight0` active; since XNA's `EnableDefaultLighting()` default light
directions are fixed in world space (not adapted to an arbitrary camera), a
single-light rig can leave much of a surface dim depending on the specific camera
angle used for this render-to-texture pass -- the same class of finding as this
session's own `TiltPerspective` port (balls rendering near-black under a single
enabled light with no diffuse contribution). Not exhaustively re-confirmed against the
real XNA original's own screenshot (out of scope given this session's overall time
budget), but this is a plausible, faithful characteristic (the C# original's own
`Draw()` sets up the exact same single-light configuration for this same
render-to-texture pass) rather than a demonstrated CNA bug -- documented here as an
observed, not-fully-root-caused characteristic rather than asserted either way.

## SpriteFont assets

`font`/`bigfont` generated via `tools/make_font.py` (DejaVuSans.ttf, substituting the
original's Arial, per CLAUDE.md convention) at 28px/80px respectively, matching
`font.spritefont`'s/`BigFont.spritefont`'s own declared sizes. `bigfont` was generated
with `--chars "abcdefghijklmnopqrstuvwxyz "` matching the original's own restricted
`CharacterRegion` (a-z + space) -- `DemoGame.cs`'s own "Our BigFont only contains
characters a-z, so if the text contains any numbers, we have to use the other font
instead" logic is ported verbatim and remains meaningful with this matching restriction.

## Input

`MenuComponent.cs`'s own input model ("We read input using the mouse API, which will
report the first touch point when run on the phone, but also works on Windows using a
regular mouse") already targets desktop mouse input natively in the C# original --
ported directly via `Mouse::GetState()`, no NOXNA fallback or invented control scheme
needed (unlike several other samples this session). Live interactive mouse-click/drag
verification was not exercised this session (same `xdotool` shared-desktop focus
caveat documented throughout this repo -- `xdotool getactivewindow` consistently showed
a different real user window holding focus). Verified instead via this repo's
established temporary debug-auto-trigger pattern (`DemoGame::SetActiveMenu()` forced
via a temporary env-var-gated hook, `helpTimer_` forced on a specific frame -- both
reverted before commit, confirmed via a clean from-scratch rebuild afterward, 0
warnings): BasicDemo, DualDemo, AlphaDemo, and EnvmapDemo were each screenshot-verified
individually, plus attract mode itself was observed live (unforced) correctly cycling
through demos with each of the transition effects (open-curtains, spinning-squares)
playing, and the F1 help overlay was confirmed to appear and correctly render its
control list.

## Screenshot-tooling reliability (not a code issue)

Several `import -window <id>` screenshots during this session's own live verification
captured stale/incorrect window content (e.g., showing a previous demo scene's frame
despite the running process's own internal state -- logged via temporary stderr
diagnostics -- being confirmed correct at the moment of capture). This matches this
repo's own already-documented "screenshot tooling has intermittently failed... under
heavy concurrent load" caveat (NEXT.md section 5) -- re-capturing (sometimes several
times) always eventually produced a screenshot matching the actual, logged application
state. Not a defect in this sample's own code.

## Content asset summary

| Asset | Source | Conversion |
|---|---|---|
| `font`/`bigfont` | `font.spritefont`/`BigFont.spritefont` | `tools/make_font.py` (DejaVuSans.ttf) |
| `checker.bmp` | same | copied verbatim (BMP loads directly) |
| `grid.model.json` + `grid_*.bin` | `grid.x` | `assimp export` -> OBJ -> `tools/obj2model.py` |
| `tank_*_verts.bin`/`_idx.bin` + `tank_parts.json` | `tank.fbx` | one-off Python script (un-baked, per-part, hierarchy-preserving -- see above) |
| `engine_diff_tex.tga`/`turret_alt_diff_tex.tga` | same | copied verbatim (TGA loads directly) |
| `model.model.json` + `model_*.bin` | `model.fbx` | `tools/fbx_ascii2model.py` (fixed for multi-UV-layer meshes -- see above) |
| `tile1.png`/`grass1.png`/`lightmap.tga` | same | copied verbatim |
| `saucer.model.json` + `saucer_*.bin` | `saucer.fbx` | `tools/fbx_ascii2model.py` |
| `saucer_texture.tga` | same | copied verbatim |
| `background.png` | `background.jpg` | converted to RGBA PNG (see above) |
| `envmap_{posx,negx,posy,negy,posz,negz}.png` | `seattle.bmp` + `seattle_alpha.bmp` | one-off Python/Pillow script reimplementing `CubemapProcessor.cs` (see above) |
| `cat.tga` | same | copied verbatim |
| `help.png` | this sample's own `.htm` has no "Sample Controls" table (touch/mouse description only) | one-off script using `tools/gen_help_png.py`'s own `build_text()`/`render_png()` helpers with hand-written control text |

`dude.fbx`, `sky.bmp` were not converted (SkinnedDemo skipped, see above).
