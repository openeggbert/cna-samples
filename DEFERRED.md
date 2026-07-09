# Deferred Implementation Items

This file tracks features, API gaps, and asset-pipeline issues that block
sample ports from compiling or running correctly.  Items are roughly ordered
by how many samples they affect.

Each item records:
- **What is missing** — the specific gap in CNA or sharp-runtime
- **Where to implement** — which repository and rough file/module
- **Blocked samples** — which sample ports are waiting on this
- **Effort estimate** — rough size: S / M / L / XL

---

## 1. XNB Asset Pipeline

**What is missing:**
XNA 4.0 samples ship with compiled `.xnb` binary assets (textures, fonts, models,
audio).  CNA does not and will not support the XNB format.

**What needs to happen instead:**
All `.xnb` assets in each sample must be converted to open formats before a
CNA sample can load them:

| XNB asset type | Replacement format |
|---|---|
| Texture2D | PNG (or any format SDL_image supports) |
| SpriteFont | CNA SpriteFont from TTF / bitmap strip |
| Model | glTF 2.0 or OBJ |
| SoundEffect | OGG Vorbis or WAV |
| Song (music) | OGG Vorbis or MP3 |
| Effect (shader) | GLSL / HLSL depending on backend |

**Where to implement:**
- Asset extraction: use `MonoGame.Content.Builder` or `mgcb` CLI to convert
  existing XNB files to source assets.
- CNA ContentManager must then load each open format directly.

**Blocked samples:** ALL samples that use Content.Load<T>()

**Effort:** M per sample (asset conversion) + L for ContentManager loader additions

---

## 2. SpriteFont Loading ✅ RESOLVED

**What was missing:**
`Content.Load<SpriteFont>("hudfont")` — CNA now fully supports this via
`.font.json` + atlas PNG format (`SpriteFontTypeReader` in `ContentManager.cpp`).

**How to generate a font asset:**
```
python3 tools/make_font.py <path/to/font.ttf> <size_px> <Content/FontName>
```
This produces `Content/FontName.font.json` + `Content/FontName.png`.

**Already used in:** SafeArea (corner labels, A-toggle hint), InputSequence (move
names, player labels, drop-shadow DrawString).

---

## 3. EasyGL: DiffuseColor ignored for VertexPositionColor meshes ✅ RESOLVED

**Was:** `EasyGLGraphicsBackend::SelectProgram` selects the GLSL shader program purely
by vertex **stride**.  `VertexPositionColor` (stride 16) → `EnsureColored3DProgram()`,
whose shader was `FragColor = vColor` with **no `uDiffuseColor` uniform**, so
`loc_diffuse = -1` and `BindDrawParams` never uploaded `BasicEffect.DiffuseColor`.
The mesh always rendered white.

**Fix:** `EnsureColored3DProgram()` now declares `uniform vec4 uDiffuseColor` and the
fragment shader outputs `FragColor = vColor * uDiffuseColor`; `prog_colored_.loc_diffuse`
is wired to it.  `GpuDrawParams.diffuseColor` defaults to `{1,1,1,1}`, so BasicEffect Ex
draws that set no diffuse are unaffected.  The non-Ex user-primitive paths
(`DrawColoredPrimitives` / `DrawIndexedColoredPrimitives`) explicitly upload white,
since they carry no diffuse and the uniform would otherwise default to 0 (black).
(`cna/src/CNA/Internal/Backends/EasyGL/EasyGLGraphicsBackend.cpp`.)

**Verified:** Primitives3D B-key cycles the tint; the cube renders red (was white).

**Effort:** S

---

## 4. EasyGL: Wireframe rendering not possible on OpenGL ES ✅ RESOLVED

**Was:** `FillMode::WireFrame` in `RasterizerState` was silently ignored — OpenGL ES 3.x
has no `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`.

**Fix:** `ApplyRasterizerState` records a `wireframe_` flag when `FillMode::WireFrame`
is set.  The 3D draw paths then call the new `DrawWireframe` helper, which re-expands
each triangle into three `GL_LINES` edges (`(a,b),(b,c),(c,a)`) through a scratch
32-bit line index buffer and draws `GL_LINES` instead of `GL_TRIANGLES`.  Covers both
indexed and non-indexed triangle list / strip draws (honouring `startIndex` /
`baseVertex` / `vertexStart`).  (`cna/.../EasyGL/EasyGLGraphicsBackend.cpp`.)

**Verified:** Primitives3D Y-key shows the cube as an edge-only wireframe.

**Effort:** M

---

## 5. VertexPositionNormal (3D lit rendering) — ✅ RESOLVED for Model-based samples (2026-07-06)

**Corrected 2026-07-06:** this item claimed CNA had no lit-shader path for
per-vertex-normal 3D rendering at all. A live check found that's no longer true:
`VertexPositionNormalTexture` (position+normal+texcoord — the vertex format
`ContentManager.cpp`'s `ModelTypeReader` already produces for any `.model.json`
with normal data, confirmed by reading the reader's `SetData` dispatch) has a real,
**tested and passing** directional-lighting path in the EasyGL backend: built and
ran `cna_test_easygl_basiceffect_combinations` live — case "(e) Directional
lighting — VertexPositionNormalTexture, white tex, red light → reddish pixel"
passes (exit code 0). The Vulkan backend also has a dedicated
`lit_textured3d.{vert,frag}.glsl` shader pair. This unblocks all 9 samples
previously listed here, since every one of them renders via `Content.Load<Model>`
(confirmed by grep of each one's own C# original) → `VertexPositionNormalTexture` →
this now-working lit path, not a bare, texture-less vertex struct:
**LensFlareSample** (#041), **Graphics3DSample** (#046, `Spaceship.cs` loads
`content.Load<Model>("Models/spaceship")` — note lowercase `content` local var
masked this from a naive `Content.Load` grep), **PickingSample** (#047 — its own
dead-code `GeometricPrimitive.cs`/bare `VertexPositionNormal` struct is *not*
compiled into the `.csproj`; the real runtime path is `Content.Load<Model>`),
**TrianglePickingSample** (#048), **HeightmapCollisionSample** (#049),
**CustomModelClassSample** (#052, loads via `Content.Load<CustomModel>` — a
sample-defined custom content type; port can just use the standard
`tools/obj2model.py` conversion + stock `Model` like every other sample, since
CNA has no generic custom-`ContentTypeReader` extensibility anyway — see item 18),
**InverseKinematics** (#057), **ChaseCamera** (#058), and **MarbleMaze**'s (#061)
EX2/End build. Update each sample's `missing.md`/`PLAN.md` status and re-attempt
porting.

**Still open — Primitives3D specifically:** Primitives3D's original C# ships its
**own**, sample-authored, texture-**less** `VertexPositionNormal` struct
(`Primitives3D/VertexPositionNormal.cs`, its own `IVertexType` — not a built-in XNA
type) for its procedurally-generated primitives (sphere/cylinder/etc. have no UV
data). CNA has no texture-less normal-lit vertex format, so this specific sample
still can't get real per-vertex lighting without either (a) a small CNA addition
(a `VertexPositionNormal` variant + wiring it into the same lit shader), or (b) a
port-side workaround (assign a dummy/unused texcoord to every vertex and use the
already-working `VertexPositionNormalTexture` instead) — the port-side workaround
is likely the pragmatic choice given `VertexPositionNormalTexture` is proven and no
second sample needs a texture-less variant.

**Effort:** — (done in `cna` for the Model-based case) / S (Primitives3D-specific
workaround, port-side, no CNA change needed)

---

## 14. TextureCube content loading (`Content.Load<TextureCube>`)

**What is missing:**
`ContentManager.cpp` has no `TextureCubeTypeReader` registered — `Content.Load<TextureCube>(...)`
throws, even though the underlying pieces it needs already exist: CNA's `TextureCube`
class and DDS decoding are both implemented and proven working (see the
`easygl_texturecube_*` example/tests), and `EnvironmentMapEffect` is also already
implemented.

**Where to implement:** Add a `TextureCubeTypeReader : ContentTypeReader<Graphics::TextureCube>`
to `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`'s built-in type readers
(same shape as the existing `Texture2DTypeReader`), reading a `.dds` cubemap file.

**Blocked samples:** RimLighting (#037) — confirmed via direct source audit
(2026-07-05) to have **zero** custom `.fx` files; it uses stock `EnvironmentMapEffect`
+ a `TextureCube` (`OutputCube.dds`) purely via `Content.Load<TextureCube>`. This is
the only real gap — unlike the rest of Phase 3, RimLighting does not need the XL
HLSL→GLSL shader pipeline (item 11) at all.

**Effort:** S

---

## 15. Accelerometer/sensor platform reality — NOT a hard blocker where a fallback fits

**What is actually true (confirmed by reading `cna/src/Microsoft/Devices/Sensors/Accelerometer.cpp`
directly, 2026-07-05):** `Microsoft::Devices::Sensors::Accelerometer` is a real,
working, SDL3 `SDL_Sensor`-backed implementation — **not** a stub. `getIsSupportedProperty()`
gates on `CNA::getCurrentPlatform()` being `Android`, `iOS`, **or** `Desktop` (not
Android-only), then does a real probe (`SDL_InitSubSystem`/`SDL_GetSensors`/
`SDL_OpenSensor`) for actual sensor hardware. On this project's development machine
(desktop Linux, no physical accelerometer chip), that probe correctly returns `false`
— that is a hardware-absence result, not a platform restriction, and the same code
path would return `true` on a real Android/iOS device or a desktop/laptop with a
physical accelerometer (e.g. a 2-in-1 tablet). `Gyroscope` has the identical
Android/iOS/Desktop-gated, real-probe shape (see `Gyroscope.cpp`).

**Established working pattern (proven three times — Yacht, SnowShovel, Bounce):**
when `getIsSupportedProperty()` is false (this desktop, the normal case), the sample
falls back to keyboard/gamepad/touch input that the *original* XNA sample's own
Windows desktop build already shipped (Yacht/SnowShovel/Bounce are all Windows Phone
ports that had a `#if WINDOWS_PHONE` accelerometer branch **and** a working non-phone
input branch in the same shipped C#) — no invented control scheme, just wiring the
already-existing fallback path to run unconditionally instead of behind a
compile-time `#if`. See `samples/Yacht/missing.md`, `samples/SnowShovel/missing.md`.

**Where this pattern does NOT directly apply — audited per-sample (2026-07-05):**
- **AccelerometerSample (#084)** and **TiltPerspective (#107):** unlike Yacht/
  SnowShovel/Bounce, these two samples' original C# ships with **no alternate input
  path at all** — the entire point of each sample is moving a sprite / shifting a 3D
  perspective purely by tilting the phone (confirmed: no `#if WINDOWS_PHONE` split, no
  keyboard/gamepad branch anywhere in `Game.cs`/`AccelerometerHelper.cs`). Porting
  either would mean *inventing* a keyboard-tilt-emulation fallback that doesn't exist
  in the original, which is a bigger deviation than Yacht/SnowShovel/Bounce needed
  (they only had to un-`#if` an existing branch) — but this project already has
  precedent for adding input schemes the original never had at all (DynamicMenu/
  UISample's touch-fallback patterns, NEXT.md §6). **Not a hard blocker — just a
  bigger design decision than usual, worth confirming with the user before doing it**,
  since "invent the missing half of the sample" is a different scope commitment than
  "port what's there."
- **Orientation (#102): this sample has nothing to do with the accelerometer at
  all** — it was miscategorized. It demonstrates `GraphicsDeviceManager
  .SupportedOrientations`/`GameWindow.CurrentOrientation`/`OrientationChanged`
  (screen rotation lock, not a physical sensor reading) — confirmed via full-text
  read of `LayoutSample.cs`/`OrientationSample.cs`: zero references to
  `Accelerometer`/`Compass`/any sensor class anywhere. CNA already has
  `DisplayOrientation.hpp`, and `GraphicsDeviceManager`/`GameWindow` both implement
  `SupportedOrientations`/`CurrentOrientation` (this is the same subsystem behind the
  already-fixed portrait-orientation bug — see the project's own
  `feedback_cna_portrait_orientation_bug` memory). **Likely portable now, pending a
  full read-through to confirm no other blocker** — PLAN.md's "Phone orientation
  sensor" reason for #102 is wrong and should be corrected/re-investigated, not
  treated as settled.
- **GeolocationSample (#095): genuinely unrelated to the accelerometer and still a
  real hard blocker** — uses `System.Device.Location.GeoCoordinateWatcher` (real
  Windows Phone GPS/network location service), with no fallback of any kind in the
  original. This one's "Phone GPS hardware" reason in PLAN.md is accurate as-is.

**Effort:** — (no CNA change needed; this item is a documentation/classification
correction, not an engineering task). Porting #084/#107 is a scope decision, not a
technical blocker. #102 needs a fresh investigation pass, not a CNA change.

---

## 6. Model Asset Conversion (FBX/X → .model.json) ✅ CNA SUPPORTS STATIC MODELS

**What is missing:**
`Content.Load<Model>()` IS implemented in CNA via `.model.json` descriptor +
binary vertex/index files, and `Model`/`ModelMesh`/`ModelBone`/`ModelMeshPart` all
work end-to-end for a **static, rigid bone hierarchy** — proven by
`examples/easygl_model_draw_test.cpp` (2-bone model, `CopyAbsoluteBoneTransformsTo`,
full `Model::Draw` chain). For any sample whose model is static (or only needs rigid
parent-child bone transforms, not skeletal animation playback), the gap is purely
**asset conversion**: XNA samples ship `.x` and `.fbx` model files that must be
converted to CNA's `.model.json` format.

**This item covers static models only.** If a sample needs to *play* a skeletal
animation (walk cycles, skinned character rigs, etc. — not just render a rigid
multi-part mesh), see item 13 below instead — that capability does not exist in CNA
yet, regardless of asset conversion.

**Caveat found while investigating SplitScreen (2026-07-04): the "asset conversion
only, no CNA code changes needed" claim above only holds for models with a *single*
bone (the common case so far — Ground, tank-as-rendered-by-CameraShake, etc., which
never move any part independently).** `examples/easygl_model_draw_test.cpp`'s 2-bone
proof constructs `ModelBone`s directly in C++ — it does **not** exercise the
`.model.json` *file format*/reader's ability to build a multi-bone hierarchy from
JSON. In reality, `ContentManager.cpp`'s `ModelTypeReader::Read()` only ever creates
one synthetic "Root" bone total; every mesh parsed from `.model.json`'s `"meshes"`
array is left with a null parent bone (`ModelMesh` doesn't even have a setter for it).
So any sample needing independently-animated rigid parts (multiple named bones, one
per moving mesh, XNA's common non-skinned "rig" pattern — e.g. a tank with rotating
wheels/turret) is blocked on a small CNA *code* change, not just conversion — see
`samples/SplitScreen/missing.md` for the full write-up (exact files/functions to
change, and why `samples/CameraShake/Content/tank.model.json` would need zero
regeneration once the reader supports it, since its mesh names already match the
bone names a consumer like `Tank.cs` expects).

**What needs to happen:**
- For single-bone (fully static) models: convert source `.x`/`.fbx` files to
  `.model.json` + binary buffers. Tools: `assimp export` + `tools/obj2model.py`, or
  `tools/fbx_ascii2model.py`. One conversion per model file per sample. No CNA code
  changes needed — proven twice (CameraShake, PerformanceMeasuring).
- For multi-bone rigid-part models (SplitScreen and similar — see above): also needs
  a `ModelMesh` parent-bone setter plus a `ModelTypeReader::Read()` change to build one
  real bone per mesh (or parse an explicit bone hierarchy from the JSON) — a CNA code
  change, not just conversion.

**Blocked samples (static geometry only, conversion-only gap):** CameraShake,
BloomSample (tank), Spacewar (Evolved ships/asteroids), ChaseCamera,
HeightmapCollision, MarbleMaze, ShipGame, and similar.

**Blocked samples (rigid multi-part bone hierarchy — needs the reader change above,
not just conversion):** SplitScreen (see its `missing.md`), SimpleAnimation,
TankOnAHeightMap, CustomModelClassSample, ModelViewerDemo (all animate the same
`tank.fbx`'s wheels/turret/cannon/hatch independently). (SkinningSample,
RolePlayingGame, and other *skinned*-animation samples are additionally blocked on
item 13 regardless.)

**Effort:** M per model (conversion) for single-bone models, no CNA code changes
needed. S–M CNA code change (once) to unblock the multi-bone rigid-part case, then M
per model as above.

**Tool bug found and fixed while porting LensFlare (2026-07-09):**
`tools/fbx_ascii2model.py` baked each mesh's raw `Vertices:`/`Normals:` data straight
into the output buffers, ignoring the source `Model` node's `PreRotation`/
`LclRotation`/`LclScaling`/`LclTranslation` properties — the node transform 3ds Max
(and similar DCC tools) bake in to convert their own internal axis convention into
the FBX file's declared one. `LensFlareSample`'s `terrain.fbx` has a `PreRotation` of
`-90,0,0` on `Plane01` (a Z-up → Y-up correction); without applying it, the "terrain"
came out standing on its edge (all height variation in Z instead of Y) and rendered
as a flat CornflowerBlue screen with nothing visible. Fixed by parsing those
properties per mesh and applying the composed rotation/scale/translation to every
position and normal before building the vertex buffers (identity transform is a
no-op, so any FBX with all-zero node properties, e.g. `tank.fbx`, converts
byte-identically to before). **Confirmed this does not change any already-shipped
asset:** every `Model::<mesh>` node in `tank.fbx` (`CameraShake`/`CustomModelClass`'s
source) has `PreRotation 0,0,0` — but several of its meshes (the wheels/turret/
canon/hatch) *do* have non-zero `Lcl Translation` values, meaning regenerating
`tank.model.json` with this fixed converter would now bake those parts into their
correct relative positions instead of leaving them stacked at the mesh's own local
origin. That regeneration was deliberately **not** done as part of this fix — it's a
separate, pre-existing-asset change outside LensFlare's own scope, and (per the
multi-bone note above) doesn't matter for CameraShake/CustomModelClass today since
neither sample moves tank parts independently of each other; only relevant once a
sample actually needs independent per-part motion (SplitScreen, SimpleAnimation, and
the other rigid-multi-part-bone-hierarchy samples listed above), at which point
regenerating `tank.model.json` with the fixed converter should be revisited.

**Addendum found while porting PickingSample (2026-07-09):** confirmed the
"no per-mesh texture" gap flagged in LensFlare's `missing.md` ("worth a small
addendum here if a future sample specifically needs a textured static model")
has a much more visually severe consequence than LensFlare's own dulled/
untextured terrain. `ModelTypeReader::Read()`'s `.model.json` mesh schema has
no `"texture"` field at all, so every mesh in PickingSample (`table`, `Sphere`,
`Cats`, `Cylinder`, `P2Wedge` — all of which have real material textures in
their source FBX files, e.g. `wood.tga`/`cat.tga`/`wedge_p2_diff_v1.tga`)
renders with `BasicEffect.TextureEnabled == false` and the class-default
`DiffuseColor` of `(1,1,1)` (plain white) instead of its real material color.
Combined with a separate, confirmed detail in `EasyGLGraphicsBackend.cpp`'s
`EnsureLit3DProgram()` fragment shader — for `VertexPositionNormalTexture`
(stride 32) draws, it unconditionally computes
`FragColor = texture(uTexture, vUV) * vec4(litRGB, uDiffuseColor.a)` with no
texture-less branch, falling back to an internal 1×1 *white* texture when
none is bound (so the multiply is a no-op) — the combination of white
`DiffuseColor` and XNA's standard bright `EnableDefaultLighting()` 3-point
rig pushes `litRGB` above `(1,1,1)` for a broad range of surface normals,
which OpenGL then clamps to solid white. The practical result, confirmed live
via screenshot at multiple camera angles: every PickingSample model renders
as a flat, fully-saturated white shape with a razor-sharp, non-antialiased
silhouette and *zero* visible shading gradient anywhere — not merely "duller"
like LensFlare's terrain, but a total loss of visual information. A correctly
bound, real-world-average-brightness texture (well under 1.0 per channel)
would pull the product back under the clamp for most pixels and restore
normal-looking shading contrast — this is a direct, if dramatic, consequence
of the existing "no per-mesh texture" gap, not an independent new lighting
bug. See `samples/PickingSample/missing.md` for the full write-up (including
confirmation, via pixel sampling across several camera angles, that this is
angle-*independent*, unlike the separate near-plane-clipping-family bug also
observed once in the same session on a different model).

**Addendum found while porting HeightmapCollision (2026-07-10):** confirmed a
second, narrower gap in `ModelTypeReader::Read()` (`ContentManager.cpp`),
independent of the texture-field gap above: it unconditionally reads every
mesh's index data as `std::uint16_t` (`idxBytes.size() /
sizeof(std::uint16_t)`, then `IndexBuffer::SetData(const std::uint16_t*, ...)`)
with no branch on vertex count and no `"indexSize"`/`"indexElementSize"` JSON
field to request 32-bit indices explicitly. Real XNA's stock `ModelProcessor`
automatically selects `IndexElementSize.ThirtyTwoBits` once a mesh exceeds
65535 vertices; a `.model.json` mesh that large has no way to express that
today. HeightmapCollision's own procedurally-generated terrain (257×257 =
66049 vertices, from `terrain.bmp`) would hit exactly this limit if routed
through `Content.Load<Model>`. **Not a hard blocker in general** — confirmed by
direct header/source read that `IndexBuffer`/`IIndexBufferBackend` (both the
EasyGL and Vulkan backends) already fully implement
`IndexElementSize::ThirtyTwoBits` end-to-end (`CreateIndexBuffer32`,
`SetData(const std::uint32_t*, ...)`, and a real `GL_UNSIGNED_INT`
`glDrawElements` path in `EasyGLGraphicsBackend::DrawIndexedPrimitivesEx`) —
the gap is specifically `ModelTypeReader`'s hardcoded 16-bit assumption, not
the underlying buffer classes. **Workaround used:** HeightmapCollision's
terrain is built directly at runtime (`Terrain.hpp`, NOXNA) instead of via
`Content.Load<Model>`, using the real 32-bit `IndexBuffer` constructor
directly — this was already necessary anyway for a different reason (no
`Model.Tag`/custom-`ContentProcessor` equivalent, item #18) — so this gap
didn't block that sample, but would block any future sample needing to
`Content.Load<Model>()` a single mesh with more than 65535 vertices.
**Where to implement (if ever needed):** `ModelTypeReader::Read()`
(`ContentManager.cpp`, in the "Meshes" parsing loop, alongside the existing
`stride`/`numVertices` computation) — read an optional `"indexElementSize"`
JSON field (or just auto-select based on `numVertices > 65535`, mirroring real
XNA's own `ModelProcessor` behavior) and branch to the `std::uint32_t`
`SetData` overload accordingly. See `samples/HeightmapCollision/missing.md`
for the full write-up.

---

## 7. Audio (SoundEffect, SoundEffectInstance, Song) ✅ RESOLVED

**What was missing:**
Audio was thought to be unimplemented but CNA fully supports audio via SDL3_mixer
(`SOUND_ENABLED`). `SoundEffect`, `SoundEffectInstance`, `Song`, `MediaPlayer`,
`SoundBank`, `WaveBank`, `AudioEngine`, and `Cue` are all implemented.

**Status:** Fully working. No blocker for audio samples.

---

## 8. SpriteBatch.DrawString (in-game text rendering) ✅ RESOLVED

**What was missing:**
`SpriteBatch.DrawString(spriteFont, text, position, color)`.

**Status:** Fully implemented in CNA. Depends on SpriteFont (item 2 above), which
is also resolved. Use `tools/make_font.py` to generate font assets, then load with
`Content.Load<SpriteFont>("FontName")`.

---

## 9. Mouse Input — Viewport.AspectRatio

**What is missing (minor):**
`Viewport` has no `AspectRatio` convenience property.  Currently computed manually
as `(float)Width / (float)Height` in each sample.

**Where to implement:** `cna/include/Microsoft/Xna/Framework/Graphics/Viewport.hpp`

**Blocked samples:** Non-blocking workaround exists; cosmetic only.

**Effort:** S

---

## 10. GamePad.IsButtonDown shortcut

**What is missing (minor):**
In XNA, `GamePad.GetState(PlayerIndex.One).Buttons.Back` is a direct member access.
In CNA, `Buttons` is accessed via `getButtonsProperty()` and `Back` via `getBackProperty()`.
`GamePadState.IsButtonDown(Buttons::Back)` works correctly and is used in the ports.

**Where to implement:** Not strictly needed; `IsButtonDown` is the cleaner API anyway.

**Effort:** —

---

## 11. Custom User Effect Shader Conversion (HLSL .fx → .shader.json) ✅ CNA SUPPORTS CUSTOM EFFECTS

**What is missing:**
`Content.Load<Effect>()` IS implemented in CNA via `.shader.json` descriptor
referencing GLSL vertex + fragment shader files.  The gap is **shader conversion**:
XNA samples use HLSL `.fx` files that must be rewritten as GLSL and described
via `.shader.json`.

**What needs to happen:**
- Translate HLSL `.fx` shader logic to GLSL (vertex + fragment)
- Create `.shader.json` descriptor per effect
- Tools: manual port or `SPIRV-Cross` + `dxc` pipeline

**Blocked samples:** BloomSample (3 shaders), DistortionSample, NonPhotoRealistic,
NormalMapping, PerPixelLighting, VertexLighting, RimLighting, ShadowMapping,
ShatterEffect, Particles3D, and all Phase 3+ shader samples.

**Effort:** XL

---

## 12. RenderTarget2D ✅ RESOLVED

**What was missing:**
`RenderTarget2D` and `GraphicsDevice.SetRenderTarget()` were thought to be missing
but are fully implemented in CNA (`RenderTarget2D.hpp` / `.cpp`, backend framebuffer
support in both EasyGL and Vulkan backends).

**Status:** Fully working. No blocker.

---

## 13. Skeletal Animation Playback (AnimationClip / Keyframe / AnimationPlayer)

**What is missing:**
Unlike items 6 and 11 (which are pure asset-conversion gaps — CNA's own runtime
already works), this is a **real, unimplemented CNA capability**. `Model`/`ModelBone`
support a static or rigid bone *hierarchy* (proven working, see item 6), but there is
no equivalent of the standard XNA "Skinned Model Sample" pipeline types:
`AnimationClip`, `Keyframe`, `AnimationPlayer` (interpolates/advances a clip's
keyframes and produces a bone-transform array per frame) do not exist anywhere in
`cna/include` or `cna/src` (confirmed by grep — zero matches). There is also no
skeletal/skinning data in the `.model.json` format itself (no bone-weight-per-vertex,
no keyframe/clip section) — `ModelTypeReader` in `ContentManager.cpp` only parses a
static `"bones"` hierarchy, not animation data.

Note this is a different problem from `SkinnedEffect`'s GLSL shader
(`src/CNA/Internal/Backends/Vulkan/shaders/skinned3d.{vert,frag}.glsl`), which already
exists and can render a skinned mesh *given* a bone-transform array — the missing
piece is producing that per-frame bone-transform array from animation data in the
first place, plus the `.model.json` schema extension to carry vertex bone
weights/indices and keyframe data.

**What needs to happen:**
- Extend the `.model.json` schema (and `ModelTypeReader`) to carry per-vertex bone
  weights/indices and a keyframe/clip section (or a sibling `.animation.json`).
- Add `AnimationClip`/`Keyframe`/`AnimationPlayer` classes (or CNA-native
  equivalents) under `cna/include/Microsoft/Xna/Framework/` mirroring the XNA
  Skinned Model Sample's own helper types, since ported samples that use skeletal
  animation generally include copies of those exact helper classes and call into
  them the same way.
- Wire `AnimationPlayer`'s output bone-transform array into `Model::Draw`'s existing
  `boneTransform` parameter (already proven working for the static case in
  `examples/easygl_model_draw_test.cpp`) — this final wiring step may be small once
  the data model and player exist.
- A conversion tool step is also needed (FBX/X skeletal animation → the extended
  `.model.json`/`.animation.json`), same class of tooling work as item 6, but cannot
  be usefully built before the runtime side above exists to consume its output.

**Blocked samples:** SkinningSample, RolePlayingGame, and any other Phase 4 sample
whose model needs to *play* an animation rather than just render a static/rigid mesh
(most of Phase 4's remaining 9 samples — check each one individually, since some may
only need static geometry and are actually blocked on item 6 alone).

**Effort:** L/XL — real engine-level feature work in `cna`, not just content
conversion. Recommend prototyping the data model + player against one sample
(SkinningSample is the XNA reference sample this pipeline is named after) before
attempting the rest.

---

## 16. Microphone capture (`Microsoft.Xna.Framework.Audio.Microphone`) ✅ RESOLVED

**Corrected 2026-07-06:** this item was added in the same session that wrote
`samples/MicrophoneEcho/missing.md`, based on general SDL3 capability knowledge
rather than an actual check of `cna`'s current source — a live check the same day
found `Microphone` fully implemented (`src/Microsoft/Xna/Framework/Audio/
Microphone.cpp`, 252 lines, `include/.../Audio/{Microphone,MicrophoneState,
NoMicrophoneConnectedException}.hpp`, real tests), merged via `feature/audio` into
`develop` on 2026-07-04 (commits like "wire Microphone::GetData()/GetQueuedBytes()
to the real stream") — **two days before this item was written**. No CNA gap
remains for MicrophoneEcho (#098); update `samples/MicrophoneEcho/missing.md` and
re-attempt the port.

**Effort:** — (done in `cna`)

---

## 17. Multiplayer networking (`Microsoft.Xna.Framework.Net` / `GamerServices.NetworkSession`) ✅ RESOLVED

**Corrected 2026-07-06:** this item was added in the same session that wrote the
NetRumble/ClientServerSample/NetworkPrediction/PeerToPeer placeholders, based on
the C# originals' API usage without actually checking `cna`'s current source for
whether it already implements `GamerServices`/`Net` — a live check the same day
found a full, real implementation: `include/Microsoft/Xna/Framework/Net/{
NetworkSession,NetworkSessionType,NetworkSessionProperties,AvailableNetworkSession,
AvailableNetworkSessionCollection,...}.hpp`, `NetworkSession.cpp` (836 lines),
`GamerServices/{GamerServicesComponent,AvatarRenderer,...}`, real LAN discovery via
`CNA::Internal::Net::ENetDiscoveryService`, and `NetworkSessionType` including
`SystemLink` (exactly what these 4 samples use) — merged via `feature/net` into
`develop` on 2026-07-04, **two days before this item was written**.

No CNA networking gap remains for ClientServerSample (#091), NetworkPrediction
(#100), or PeerToPeer (#103) — update their `missing.md` files and re-attempt
porting. NetRumble (#062) is now only single-blocked, by item 11 (its 4 custom
`.fx` shaders, including the bloom post-process) — update its `missing.md` to drop
the networking half of its "double-blocked" framing.

**Update, same day, after actually porting ClientServerSample:** "the types exist
and are tested" turned out to be true but incomplete — three more specific,
narrower gaps surfaced only once a real sample was built and *run* against them,
each documented as its own item since they're independently fixable: item 19
(`GamerServicesDispatcher::Update()` no-op hangs the synchronous `Create`/`Find`/
`Join` wrappers whenever a `GamerServicesComponent` is present — i.e., whenever a
sample matches its own C# original's real usage), item 20 (`NetworkGamer.IsHost`/
`.Id` are hardcoded stub constants, not per-instance state), and item 21 (the
initial `GamerJoined` event is queued for the next frame instead of raised
synchronously during `Create()`/`Join()`, unlike real XNA). ClientServerSample
works around all three at the sample level (see its `missing.md`) and is fully
ported and live-verified; the other three networking samples likely need the same
workarounds but haven't been individually re-confirmed yet. **Lesson reinforced:**
even a "the API surface exists and has tests" confirmation isn't the same as "a
real caller integrating it works" — building and running an actual sample against
new API surface can surface gaps that isolated unit tests didn't exercise.

**Effort:** — (done in `cna`) for the core types; see items 19–21 for the narrower
gaps found integrating them.

---

## 18. Content-pipeline processor extensibility (build-time `ContentProcessor` chaining)

**What is missing:**
CNA's entire asset story is "convert once, offline, with a standalone tool, into a
static runtime JSON/binary format" (`tools/obj2model.py`, `tools/make_font.py`,
`tools/gen_help_png.py`, etc.). There is no pluggable, MSBuild-time,
C#-`ContentProcessor`-style extensibility point — nothing resembling XNA's
`ContentProcessor<TInput,TOutput>` / `ContentProcessorContext.Convert`/`BuildAsset`
chaining model, where a sample can supply its own processor(s) that run as part of
the content build and transform/synthesize data before it ever reaches the runtime
(e.g. baking a reflection cubemap from a flat photo, or flattening/re-deriving model
data in a project-specific way).

This is a different, deeper class of gap than items #6 (static model format
conversion) or #11 (custom shader conversion) — both of those assume an offline tool
already produced a static input file; this item is about the *meta*-capability of a
custom build-time transform pipeline itself.

**Blocked samples:** CustomModelEffect (#053) — its `CustomModelEffectPipeline`
project chains three custom processors (`EnvironmentMappedModelProcessor` →
`EnvironmentMappedMaterialProcessor` → `CubemapProcessor`) to synthesize a 6-face
reflection cubemap from a single flat photo at build time; see
`samples/CustomModelEffect/missing.md` for the full breakdown. No other audited
Phase 3/4 sample needs this — every other custom-`.fx` sample (BloomSample,
NormalMapping, etc.) applies its effect entirely at runtime via an ordinary
`Content.Load<Effect>()` call, with no custom `ContentProcessor` involved.

**Effort:** L — most likely an informal one-off offline preprocessing script (Python
or C++) that performs the same steps against this sample's specific assets and emits
a `.model.json` + cubemap file CNA's runtime can load directly, rather than a general
pluggable pipeline (no second sample has demonstrated a need for genuine
extensibility yet).

---

## 19. `NetworkSession::Create`/`Find`/`Join` hang forever when a `GamerServicesComponent` is added ✅ RESOLVED

**Resolved 2026-07-06** in `cna`'s `feature/net` (commit `08171ac`, Task 12.1):
`NetworkSessionAction` now completes the instant it's constructed (every `Begin*`
already does all its real work synchronously in `End*`, so there's no genuine pending
operation to wait on) — confirmed this hang is a real bug in FNA's own reference
source too (`GamerServicesDispatcher.Update()` is equally an empty no-op there), not
just a CNA porting defect. ClientServerSample's workaround (omit
`GamerServicesComponent`) was removed the same day; the component is now constructed
and added normally, matching the C# original, and confirmed live (no hang, real
`GamerServicesDispatcher::Initialize()` now populates `Gamer::SignedInGamers` — see
`samples/ClientServerSample/missing.md`'s Verification section). NetworkPrediction
(#100)/PeerToPeer (#103) can now add `GamerServicesComponent` normally too when
ported; not yet separately re-confirmed for those two.

**What is missing:** confirmed live (2026-07-06) while porting ClientServerSample
(#091): calling `NetworkSession::Create(...)` after constructing a
`GamerServicesComponent` and adding it to `Game.Components` — exactly what every one
of these samples' C# originals do in their constructor
(`Components.Add(new GamerServicesComponent(this));`) — hangs forever in a tight
busy-loop, 99% CPU, no output, unresponsive to SIGTERM. Root cause, confirmed by
reading the source directly: `NetworkSession::Create()`'s synchronous wrapper is
```cpp
System::IAsyncResult* result = BeginCreate(...);
while (!result->getIsCompletedProperty())
{
    if (!GamerServices::GamerServicesDispatcher::UpdateAsync())
        activeAction_->setIsCompletedProperty(true);
}
return EndCreate(result);
```
`GamerServicesDispatcher::UpdateAsync()` is `{ if (isInitialized_) Update(); return isInitialized_; }`, and `GamerServicesDispatcher::Update()` is a **completely empty function body** — it does nothing at all, ever. `BeginCreate()` never sets `IsCompleted` itself (it just constructs a `NetworkSessionAction` and returns). So: with no `GamerServicesComponent` (dispatcher never initialized), `UpdateAsync()` returns `false` on the very first loop iteration, which forces `IsCompleted = true`, and the loop exits immediately (this is *why* CNA's own `NetworkSessionTests.cpp` — which never constructs a `GamerServicesComponent` — doesn't hit this). But with a `GamerServicesComponent` added (matching every sample's own C# original), `isInitialized_` becomes `true`, `UpdateAsync()` unconditionally returns `true` forever, and nothing — nowhere in the codebase — ever sets `IsCompleted` on that pending action. The loop never exits.

**Where to implement:** `cna/src/Microsoft/Xna/Framework/GamerServices/GamerServicesDispatcher.cpp`'s `Update()` needs to actually do something when a `NetworkSession` action is pending — at minimum, poll the ENet backend for whether the requested operation (host bind, LAN session discovery, join handshake) has completed and call `activeAction_->setIsCompletedProperty(true)` once it has, mirroring what a real synchronous completion would look like.

**Workaround used in ClientServerSample (#091):** don't construct/add a
`GamerServicesComponent` at all. Confirmed this loses no other functionality in this
CNA implementation (the component has no other observable effect — `Guide.ShowSignIn`
is independently a no-op regardless of whether the component exists, per item 15-
adjacent findings). This is a real, documented deviation from the C# original (see
`samples/ClientServerSample/missing.md`), not a silent workaround.

**Blocked samples:** ClientServerSample (#091, workaround applied, ported), and
presumably NetworkPrediction (#100), PeerToPeer (#103), NetRumble (#062) — all four
call `NetworkSession::Create`/`Find`/`Join` the same synchronous way and all four
C# originals add a `GamerServicesComponent`; the same workaround (omit it) should
apply, but hasn't been separately confirmed live for the other three yet.

**Effort:** M — `GamerServicesDispatcher::Update()` needs real logic, not a bigger
architectural change.

---

## 20. `NetworkGamer` identity is not distinguishable (`IsHost` always true, `Id` always 0) ✅ RESOLVED

**Resolved 2026-07-06** in `cna`'s `feature/net` (commit `81f10b5`, Task 12.2):
`NetworkGamer` gained real `SetId`/`SetIsHost` (NOXNA), wired through
`NetworkSession`'s constructor (host vs. joined-client state) and through
`ENetBackend.cpp`'s already-existing real, cross-machine-consistent wire-id system
(it just wasn't surfaced through the public API before this fix). ClientServerSample's
`bool isHost_` local-tracking workaround was removed the same day; every call site now
uses `networkSession_->getIsHostProperty()`/`gamer->getIsHostProperty()` directly,
confirmed live for the solo-host case (see `samples/ClientServerSample/missing.md`'s
Verification section). **Scoped, still-open limitation, tracked here rather than as a
new item:** a *remote* gamer representing the actual host machine still reports
`IsHost == false` as seen from a client (`RosterEntry` carries no host flag on the
wire) — a strict improvement over "every gamer said true," not a full fix; would need
a wire-protocol addition to close. Genuine multi-gamer `Id`-routing within
ClientServerSample itself wasn't re-verified live this session (blocked by an
unrelated, pre-existing `ENetDiscoveryService` two-process discovery limitation on
this container — see missing.md) but is proven at the `cna` test-suite level
(`NetworkSessionTests.cpp`, `ENetBackendTests.cpp`, `TwoProcessLoopbackTest.cpp`).

**What is missing:** confirmed by reading `cna/src/Microsoft/Xna/Framework/Net/
NetworkGamer.cpp` directly:
```cpp
bool NetworkGamer::getIsHostProperty() const              { return true; }
SharpRuntime::bytecs NetworkGamer::getIdProperty() const  { return 0; }
```
Both are **hardcoded stub constants**, not derived from any actual state (doc
comments call this "matching FNA's stub" — plausible for real FNA/XNA without a live
Xbox LIVE/GfWL backing service, but it means every `NetworkGamer` in a session
— host and every client — reports `IsHost == true` and `Id == 0`, with no way to
tell them apart via these two properties. Consequences, both confirmed by reading
the code (not merely inferred):
- `NetworkSession::getIsHostProperty()` is implemented as "true if any local gamer's
  `IsHost` is true" — since that's unconditionally true, `NetworkSession.IsHost` is
  **also always true**, on every machine, host or client.
- `NetworkSession::FindGamerById(id)` does a linear `getIdProperty() == id` scan —
  since every gamer's `Id` is 0, this **always returns the first gamer in
  `AllGamers`**, regardless of which `id` was actually requested. Any protocol that
  writes `gamer.Id` into a packet and looks it back up on the receiving end (e.g.
  ClientServerSample's/NetworkPrediction's/PeerToPeer's per-tank state sync) will
  misroute every gamer's state onto the first gamer whenever more than one gamer is
  in the session.

**Workaround used in ClientServerSample (#091):** for "am I the host" (session-level),
track a local `bool isHost_` set explicitly at the call site — `true` after
`NetworkSession::Create()`, `false` after `NetworkSession::Join()` — since each
machine always knows unambiguously which one it called, this fully replaces the
broken `NetworkSession.IsHost`/`gamer.IsHost` for this sample's needs. No workaround
was applied for `FindGamerById`/`Id` — the port keeps the original's `gamer.Id`-based
wire protocol as-is (matching the C# source), so **multi-gamer sessions will not
correctly route tank state past the first gamer** until this is fixed in `cna`; this
is a known, documented, unfixed limitation for that one scenario (see
`samples/ClientServerSample/missing.md`). Single-gamer (solo host, no other clients
joined) sessions are unaffected and were live-verified working.

**Where to implement:** `NetworkGamer` needs a real per-instance identity — e.g. an
`isHost_` bool member correctly set at construction (mirroring `NetworkSession::
host_`, which *is* tracked correctly) and returned by `getIsHostProperty()` instead of
a hardcoded `true`; and a real per-session-unique `byte` id assigned when each gamer
joins (local or remote) and returned by `getIdProperty()` instead of a hardcoded `0`.

**Blocked samples:** ClientServerSample (#091, `IsHost` worked around, `Id`/
`FindGamerById` not), and likely NetworkPrediction (#100), PeerToPeer (#103),
NetRumble (#062) to varying degrees depending on whether each protocol relies on
`gamer.Id`/`FindGamerById` for multi-gamer state routing — not separately confirmed
for those three yet.

**Effort:** M.

---

## 21. Initial `GamerJoined` event is queued for the next `Update()`, not raised synchronously during `Create()`/`Join()` ✅ RESOLVED

**Investigated 2026-07-06** in `cna`'s `feature/net` (Task 12.3) — initially found
**not** to have a simple fix. Traced against this exact sample's real C# reference
source (`ClientServerGame.cs`) and confirmed **both originally-proposed fix
approaches below could not actually work, and either would have been an active
regression**: subscription (`HookSessionEvents()`) always happens strictly *after*
`Create()`/`Join()` already returned control to the caller (there is no way to
subscribe any earlier — the session pointer doesn't exist yet), so raising the event
any earlier than that fires into zero subscribers — including inside the constructor
or inside `Create()`/`Join()`'s own wrapper before returning. Worse, doing so would
have broken ClientServerSample's own already-working, live-verified workaround below,
which depended on the event still being queued (not yet fired into a void) at the
point it calls `Update()` right after subscribing.

Real XNA's `NetworkSession.GamerJoined` is documented to replay itself immediately
upon `+=` subscription for every gamer already present in the session — a "hot event
with backlog replay" semantic that CNA's `System::EventHandler<T>` (a separate
`sharp-runtime` repo, not `cna` itself) had no hook for.

**Resolved same day, after the user reviewed this exact analysis and approved a
`sharp-runtime` change:** `EventHandler<T>` gained a generic, opt-in
`SetReplayHook()` (`sharp-runtime` `develop`, commit `69661c2`) — every other
`EventHandler<T>` in the codebase that never calls it behaves exactly as before, so
this isn't the one-off/non-uniform special case that was ruled out above.
`NetworkSession`'s constructor (`cna`'s `feature/net`, commit `ab05395`) now sets this
hook so `GamerJoined += handler` immediately replays for every gamer already in the
session, matching real XNA. ClientServerSample's `networkSession_->Update();`
workaround (right after `HookSessionEvents()` in both `CreateSession()`/
`JoinSession()`) was removed and confirmed live to still work correctly with no
crash (see `samples/ClientServerSample/missing.md`'s Verification section for the
full account, including how `xdotool` input unreliability was ruled out as the cause
of anything via a controlled debug-auto-trigger test). See `cna`'s `plan_net.md` Task
12.3 for the full investigation and fix.

**What is missing:** in real XNA, `NetworkSession.Create()`/`.Join()` synchronously
establish the initial local gamer(s) and raise `GamerJoined` for each one as part of
that same call — by the time `Create()` returns, code relying on `GamerJoined`
having already fired (e.g. a `Tag` set by the handler) can safely assume it's done.
CNA's `NetworkSession` constructor instead *queues* a `GamerJoin` `NetworkEvent` per
initial gamer into an internal `std::queue`, which is only drained by
`NetworkSession::Update()` — i.e., not until the *next* frame's `networkSession.
Update()` call in the ported sample's own game loop, one full `Update()` cycle after
`Create()`/`Join()` returns.

**Consequence, confirmed live:** a sample whose `Update()` loop is structured like
the original (read/act on each local gamer via its `Tag` *before* calling
`networkSession.Update()` — exactly ClientServerSample's `UpdateNetworkSession()`
shape) will find an empty `Tag` (`std::any` holding nothing) on the very first
network-session frame, since the handler that populates it hasn't run yet. Observed
as an uncaught `std::bad_any_cast` terminating the process.

**Workaround used in ClientServerSample (#091):** call `networkSession_->Update();`
once, immediately after `HookSessionEvents()`, inside both `CreateSession()` and
`JoinSession()` — this drains the queued initial `GamerJoin` event(s) synchronously
before control returns to the normal per-frame loop, matching what real XNA does
implicitly. Confirmed live: with this fix, session creation + tank spawn + render
all work end-to-end with no crash (screenshot-verified).

**Where to implement:** `NetworkSession`'s constructor could raise `GamerJoined`
directly for each initial local gamer instead of only queuing a `NetworkEvent`, or
`Create()`/`Join()`'s synchronous wrappers could drain the queue once before
returning — either would remove the need for every calling sample to work around it.

**Blocked samples:** ClientServerSample (#091, workaround applied, ported); likely
also affects NetworkPrediction (#100) and PeerToPeer (#103) if they have a similar
"read local gamer state before pumping the session" loop shape — not separately
confirmed for those two yet.

**Effort:** S.

---

## 22. EasyGL backend ignores `BlendState.ColorWriteChannels` (color-write mask never applied)

**Found while porting LensFlare (2026-07-09).** `LensFlareComponent`'s occlusion-query
trick (`LensFlareComponent.cs`'s `UpdateOcclusion()`) relies on a custom `BlendState`
with `ColorWriteChannels = ColorWriteChannels.None` so the query polygon it draws to
test sun visibility never actually appears on screen — only its occluded/visible
pixel *count* matters. Confirmed via direct source grep:
`grep -rn "ColorWriteChannels\|glColorMask" src/CNA/Internal/Backends/EasyGL/` in
`cna` returns **zero** matches — the EasyGL backend parses/stores
`BlendState.ColorWriteChannels` (and `...Channels1/2/3`) but never calls the
OpenGL-ES equivalent (`glColorMask`) to actually apply it, so every draw call writes
all four color channels regardless of the active `BlendState`.

**Confirmed live:** running the ported sample shows a solid opaque white square (the
occlusion-query polygon, `querySize` × `querySize` at the current light-screen
position) that should be invisible, on top of the CornflowerBlue background and the
already-known near-plane-clipping thin-line artifact (item 5's terrain-asset
variant — same shared framework cause as the tank models, confirmed by the terrain
now converting/orienting correctly per item 6's write-up above, so the thin line is
not an asset problem).

**Root cause:** `CNA::Internal::Backends::EasyGL::EasyGLGraphicsBackend` has no
`glColorMask` call anywhere in its state-application path.

**Where to implement:** wherever the EasyGL backend applies `BlendState` to GL state
(alongside its existing blend-func/blend-equation setup) — add a `glColorMask` call
driven by `BlendState.ColorWriteChannels` (all four channels come from the same
`ColorWriteChannels` value unless `IndependentBlendEnable`/per-target overrides are
also unimplemented, which is a separate, likely-also-missing MRT feature not
investigated here).

**Blocked samples:** LensFlare (#041, ported; ported using the assumption that fixing
this is a follow-up, not a blocker for shipping the port — see its `missing.md`).
Any future sample using a `ColorWriteChannels.None`/partial-channel `BlendState` for
a similar depth-only or stencil-only trick would hit the same gap.

**Effort:** S.

---

## 23. `Game::DoInitialize()` wires up `Components_.ComponentAdded` *after* calling the user's `Initialize()` override

**Found while porting Graphics3D (2026-07-09).** Real XNA/FNA's `Game`
constructor subscribes to `Components.ComponentAdded`/`ComponentRemoved`
immediately, before any user `Initialize()` override can run — this is why
real XNA supports the common pattern of creating `DrawableGameComponent`s and
calling `Components.Add(...)` from *inside* `Initialize()` (not just the
constructor), which is exactly what Graphics3D's C# original does
(`GameMain.cs`'s `CreateLightEnablingButtons()` etc., all called from
`Initialize()`).

**Confirmed live:** `cna`'s `Game::DoInitialize()`
(`src/Microsoft/Xna/Framework/Game.cpp`) calls `Initialize()` first, and only
wires up `Components_.ComponentAdded`/`ComponentRemoved` afterward
(`DoInitialize()`'s own body, after the `Initialize()` call). A component
added to `Components` from within `Initialize()` is added to the collection
(and later categorized as updateable/drawable, so it does run every frame) but
its own `Initialize()` — and therefore `LoadContent()`, since
`DrawableGameComponent::Initialize()` calls it — is never invoked, since the
event that would trigger it isn't subscribed yet. Reproduced directly: a
`Checkbox` (`DrawableGameComponent`) added this way had an unset
`std::optional<SpriteBatch>`/`std::optional<Texture2D>` at first `Draw()` —
segfault on the very first frame.

**Root cause:** `Game::DoInitialize()` subscribes the `ComponentAdded`/
`ComponentRemoved` event handlers after calling `Initialize()`, not before —
deviates from FNA/XNA's own `Game` constructor, where the equivalent
subscription happens immediately.

**Where to implement:** move the `Components_.ComponentAdded +=`/
`ComponentRemoved +=` subscription earlier — either into `Game`'s constructor
(matching FNA exactly) or at minimum to before the `Initialize()` call inside
`DoInitialize()`.

**Workaround used in Graphics3D:** an `AddComponent(Checkbox*)` helper that
calls `Components.Add(component)` followed by an explicit
`component->Initialize()`. Safe even after a `cna` fix, since
`DrawableGameComponent::Initialize()` already guards against
double-initialization.

**Blocked samples:** Graphics3D (workaround applied, ported). PickingSample
(#047) hit the identical shape and applied the identical workaround. Every
other sample in this repo happens to add its components from the
*constructor* (before `Initialize()` runs at all), which sidesteps this gap
entirely. Any future sample following the same pattern (matching a C#
original that does its own component creation inside `Initialize()`) would
hit the same bug.

**Addendum found while porting TrianglePicking (2026-07-09): the precise
trigger condition is narrower than "added from `Initialize()` vs. the
constructor" — it's about ordering relative to the *user override's own call*
to `Game::Initialize()`/`base.Initialize()`.** `cna`'s base `Game::Initialize()`
(`Game.cpp`, not `DoInitialize()`) separately, unconditionally loops over every
component **already present** in `Components_` at the time it runs and calls
each one's own `Initialize()` directly — entirely independent of whether
`ComponentAdded` has been subscribed yet. Graphics3D's and PickingSample's own
C# originals both add their component(s) from inside `Initialize()` **before**
that override's own call to `base.Initialize()`, so the base loop's snapshot
doesn't include them yet (the loop already ran) and only the — not-yet-
subscribed — `ComponentAdded` event could have caught them. TrianglePicking's
own C# original instead adds its `Cursor` from the **constructor**, i.e. long
before `Initialize()`/`base.Initialize()` runs at all — confirmed live, this
needs **no workaround**: by the time either `Game::Initialize()` (base or
override) executes, the component is already present in `Components_`, so the
base loop's unconditional pass initializes it correctly with no dependency on
`ComponentAdded` timing at all. In short: constructor-time adds are safe (base
`Game::Initialize()`'s own loop catches them); `Initialize()`-time adds are
only safe if they happen **after** that override's own `base.Initialize()`
call (untested by any sample so far — every sample hitting this pattern in C#
calls `base.Initialize()` last, matching real XNA convention). See
`samples/TrianglePicking/missing.md` for the full account.

**Effort:** S.

---

## 24. `GraphicsDevice::Clear(Color)` (single-argument overload) never clears the depth buffer

**Found while porting Graphics3D (2026-07-09)**, while investigating that
sample's invisible-model bug (item 5's near-plane-clipping-family entry — this
turned out not to be the cause of that specific bug, but is a real, separate
gap). Real XNA's `GraphicsDevice.Clear(Color)` convenience overload clears the
color target, depth buffer, and stencil buffer together (documented behavior,
matches FNA).

**Confirmed via direct source read** (`src/Microsoft/Xna/Framework/Graphics/
GraphicsDevice.cpp`): the single-argument `Clear(const Color&)` overload only
ever calls `backend_->Clear(r,g,b,a)` (color only) — it never touches depth or
stencil. The `Clear(ClearOptions, const Color&, float depth, int stencil)` and
`Clear(const Color&, float depth)` overloads both correctly clear depth
(`ClearColorAndDepth`) when asked; only the plain single-`Color`-argument
overload is missing it.

**Root cause:** `GraphicsDevice::Clear(const Color&)` doesn't forward to the
`ClearOptions`-based overload with `DepthBuffer` included — it's a strict
subset of what real XNA's same-signature overload does.

**Impact:** every 3D sample calling `device.Clear(SomeColor)` at the top of its
own `Draw()` (i.e. essentially every 3D sample in this repo — `CameraShake`,
`CustomModelClass`, `LensFlare`, `Graphics3D`) draws each frame against a depth
buffer that was never actually cleared by that call, relying on whatever the
driver/backend leaves behind from the previous frame (or an uninitialized
buffer on the first frame). Tested substituting the two-argument
`Clear(color, 1.0f)` overload in Graphics3D specifically to see if this
explained that sample's invisible spaceship — it didn't change the result, so
this is confirmed independent of item 5's near-plane-clipping bug, not a
duplicate finding.

**Where to implement:** `GraphicsDevice::Clear(const Color& color)` in
`GraphicsDevice.cpp` should forward to
`Clear(ClearOptions::Target | ClearOptions::DepthBuffer | ClearOptions::Stencil, color, 1.0f, 0)`
(or equivalent), matching real XNA's documented behavior for this overload.

**Blocked samples:** none outright (every affected sample still renders
*something*, since GPU depth buffers are typically cleared to a sane default
by the driver on allocation in practice) — but this is a latent correctness
gap that could cause intermittent, driver-dependent depth artifacts in any 3D
sample using this Clear overload, and is worth fixing on general principle.

**Effort:** S.

---

## 25. `VertexBuffer`/`IndexBuffer` have no `GetData()` (no readback of GPU buffer contents)

**Found while porting TrianglePicking (2026-07-09).** This sample's C# original
(`TrianglePickingSample`) needs its models' raw triangle vertex positions at runtime
for a per-triangle ray-intersection test — real XNA gets this via a custom
content-pipeline processor (`TrianglePickingProcessor`, a `ModelProcessor` subclass)
that walks the model's node tree at content-**build** time and attaches a flat
`Vector3[]` (plus a precomputed `BoundingSphere`) to `Model.Tag`. The task brief for
this port suggested an alternative if that path weren't available: read the data back
from the model's already-loaded `VertexBuffer`/`IndexBuffer` at runtime instead (real
XNA's `VertexBuffer.GetData<T>()`/`IndexBuffer.GetData<T>()`).

**Confirmed via direct, full read of both headers** (`cna/include/Microsoft/Xna/
Framework/Graphics/{VertexBuffer,IndexBuffer}.hpp`): **neither class has any `GetData`
method at all** — every data-transfer method on both classes is a `SetData`/
`SetDataRaw`/`SetDataWithOptions` upload path (grep confirms zero occurrences of
`GetData` in either file). This means there is currently **no way to read back vertex
or index data from a GPU buffer in CNA**, for any purpose — not just an inconvenience
specific to this sample's `Model.Tag` gap (DEFERRED.md item #18, custom
`ContentProcessor` extensibility), but a distinct, narrower missing capability: even a
sample willing to re-derive its own picking data at runtime from an already-loaded
`Model`'s buffers has no API surface to do so.

**Where to implement:** add `GetData<T>(T* data, int count)` (and the `startIndex`/
`elementCount` overloads, matching the existing `SetData` overload set) to both
`VertexBuffer` and `IndexBuffer` — reading back from the backend's GPU buffer object
(`glGetBufferSubData` on the EasyGL/OpenGL-ES backend; note OpenGL ES 3.x *does*
support `glGetBufferSubData`, unlike some other ES-vs-desktop-GL gaps this repo has
hit, so this should not need a new capability probe). `CNA::Internal::Backends::
IVertexBufferBackend`/`IIndexBufferBackend` would need a matching virtual method added
for each backend (EasyGL, Vulkan) to implement.

**Workaround used in TrianglePicking (#048):** since CNA's whole asset story is
"convert once, offline, into a static runtime format" (item #18's framing) rather than
XNA's build-time `ContentProcessor` chaining, this port extends `tools/
fbx_ascii2model.py` (which already parses every mesh's raw triangle/vertex data to
produce `.model.json`'s own `_verts.bin`/`_idx.bin` files) with an optional `--picking`
flag that, from that same already-parsed data, additionally emits a flat binary
sidecar file (`<Model>_picking.bin`: triangle-expanded `float32 x,y,z` positions, no
normal/uv, 9 floats per triangle) — this is generated once, offline, at asset-
conversion time, and the C++ port just reads it directly from disk at `LoadContent()`
time (see `samples/TrianglePicking/src/TrianglePickingData.hpp`). This sidesteps the
gap entirely rather than working around it inside CNA, and needed no changes to
`ModelTypeReader`/`Model.Tag`-equivalent machinery. A future sample that specifically
needs to read back data from a `Model` it did **not** convert itself (e.g. a
third-party/pre-existing `.model.json` with no matching source FBX available at
runtime) would still need the real `GetData()` fix above.

**Blocked samples:** none outright — the tool-level workaround fully unblocks
TrianglePicking (#048). Any future sample needing genuine runtime readback of GPU
buffer contents (not just triangle data producible by this repo's own offline
converters) would hit this gap for real.

**Effort:** S–M (mirrors the existing `SetData` overload set; the GPU-side readback
call itself is a single line per backend on both EasyGL and Vulkan).

---

## 26. `ModelTypeReader::Read()` uploads corrupted vertex data for every stride-32 `.model.json` (vtable-size mismatch) — likely the true cause of the "near-plane-clipping" bug family

**Found while porting InverseKinematics (2026-07-10).** A straightforward first port of
this sample's `cylinder.x` (converted via `assimp export` + `tools/obj2model.py`, the
exact same pipeline already proven for CameraShake/PerformanceMeasuring/Graphics3D) built
and loaded without error — confirmed via debug instrumentation: `Content.Load<Model>
("cylinder")` produced exactly 1 mesh, 1 `ModelMeshPart` (418 vertices, 190 primitives),
and a correctly-linked `BasicEffect` — but **the model never rendered**, at any camera
distance/scale, with `RasterizerState::CullNone`, with lighting forced off, and even
reduced to a single full-scale, identity-world, unlit, un-textured triangle drawn through
the exact same `Model`/`ModelMesh::Draw()` path. A hand-built triangle at the same scale
and **PickingSample's own already-shipped, already-working `Cylinder.model.json`** (loaded
fresh into this sample's own code) **both failed to render the same way**, ruling out this
mesh's own data, the camera, and render state as the cause. A large model
(HeightmapCollision's `sphere.model.json`, 3252 vertices) rendered *correctly* through the
identical code path at every scale tested (including scaled down to match the failing
tests' size) — the key clue that the *reader*, not any one sample's own code or a specific
asset, was responsible, and that it was somehow size/vertex-count sensitive.

**Root cause, confirmed by direct source read plus a standalone `sizeof()` probe:** every
vertex struct in CNA (`VertexPositionColor`, `VertexPositionTexture`,
`VertexPositionColorTexture`, `VertexPositionNormalTexture`) publicly inherits from the
polymorphic `Microsoft::Xna::Framework::Graphics::IVertexType`
(`include/Microsoft/Xna/Framework/Graphics/IVertexType.hpp`:
`virtual ~IVertexType() = default;` plus a pure virtual
`getVertexDeclarationProperty()`), so every instance carries an 8-byte vtable pointer that
XNA's own "clean" (unpadded) vertex layouts never had. A standalone `sizeof(T)` probe
(`#include`s only `cna`'s own headers, no linking against any implementation needed since
layout is fully determined by the declarations) confirms this live:

```
sizeof(VertexPositionColor)         = 40   (XNA/clean size: 16)
sizeof(VertexPositionTexture)       = 32   (XNA/clean size: 20)
sizeof(VertexPositionColorTexture)  = 56   (XNA/clean size: 24)
sizeof(VertexPositionNormalTexture) = 40   (XNA/clean size: 32)
```

`ModelTypeReader::Read()` (`cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`,
the "Meshes" parsing loop) selects which typed `VertexBuffer::SetData` overload to call
for a mesh by comparing the `.model.json`-declared `"vertexStride"` (always one of the
*clean* XNA sizes 16/20/24/32, since every conversion tool in this repo —
`tools/obj2model.py`, `tools/fbx_ascii2model.py` — writes the tightly-packed layout, not
the padded/vtable-inflated one) against `sizeof(...)` of these now-inflated structs:

```cpp
if (stride == static_cast<int>(sizeof(Graphics::VertexPositionColor)))        // 40, never 16
    vb->SetData(reinterpret_cast<const Graphics::VertexPositionColor*>(vertBytes.data()), numVertices);
else if (stride == static_cast<int>(sizeof(Graphics::VertexPositionNormalTexture))) // 40, never 32
    vb->SetData(reinterpret_cast<const Graphics::VertexPositionNormalTexture*>(vertBytes.data()), numVertices);
else if (stride == static_cast<int>(sizeof(Graphics::VertexPositionColorTexture)))  // 56, never 24
    vb->SetData(reinterpret_cast<const Graphics::VertexPositionColorTexture*>(vertBytes.data()), numVertices);
else if (stride == static_cast<int>(sizeof(Graphics::VertexPositionTexture)))       // 32 !!
    vb->SetData(reinterpret_cast<const Graphics::VertexPositionTexture*>(vertBytes.data()), numVertices);
```

None of the declared "clean" stride values (16/20/24/32) equal their *intended* struct's
real (vtable-inflated) size any more — **except** `"vertexStride": 32`, the value **every**
`Content.Load<Model>`-based sample in this entire repo uses (since `obj2model.py`/
`fbx_ascii2model.py` only ever emit `VertexPositionNormalTexture` data), which
*accidentally* equals `sizeof(VertexPositionTexture)` (also 32, by coincidence of the
specific field-count/padding arithmetic — a real `VertexPositionTexture` is
8-byte-vtable + 12-byte `Position` + 8-byte `TextureCoordinate` = 28, padded up to 32 for
4-byte alignment). So the reader always silently dispatches to the **wrong** branch: it
`reinterpret_cast`s the raw, vtable-free file bytes (laid out as `position[12] +
normal[12] + texcoord[8]`, no vtable, exactly matching what the offline tools wrote) as an
array of real, vtable-shifted `VertexPositionTexture` objects, and reads each vertex's
`.Position`/`.TextureCoordinate` fields from the **wrong byte offsets** relative to the
source buffer (a real `VertexPositionTexture` object has its vtable pointer at offset 0,
`Position` at offset 8, `TextureCoordinate` at offset 20 — none of which match the file's
actual offsets 0/12/24) — silently corrupting the position/texcoord data uploaded to the
GPU for **every single stride-32 `.model.json` in this entire repository**, not just this
one sample's asset.

**Why this is very likely the true root cause of the long-tracked "near-plane-clipping"
bug family (section 4/5 of NEXT.md, this item's own predecessor investigations across
CameraShake/CustomModelClass/LensFlare/Graphics3D/PickingSample/TrianglePicking):** a
corrupted, effectively-arbitrary per-vertex byte-offset reinterpretation of a mesh's own
position data would produce exactly the two symptoms already observed and attributed to
"near-plane clipping" — a degenerate thin line (if the corrupted positions still land
roughly in the same region of space, just scrambled) or full invisibility (if the
corrupted positions land far outside the view frustum) — and does so *without* needing any
actual clip-space/projection defect at all. No prior session ever directly inspected the
EasyGL clipping code itself before attributing the symptom to it; this bug, by contrast,
is confirmed by direct source read and a live `sizeof()` probe, and directly, reproducibly
fixed here by bypassing the reader entirely (see below). **Not re-attributed with 100%
certainty this session** — conclusively settling it for the *other* affected samples would
mean re-testing `tank.model.json`/`terrain.model.json`/the spaceship's converted `.obj`
through the same typed-`SetData` bypass used here, which is out of this task's own scope —
but this is flagged as by far the most likely explanation, and a natural, well-scoped next
step for whoever picks up section 8 task 2 (the "investigate near-plane clipping" task)
next: **read this item first**, and try the bypass in one already-affected sample
(CameraShake is the smallest) before assuming the bug is really in clip-space math.

**Where to implement (if fixed in `cna`):**
`cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`'s `ModelTypeReader::Read()`
needs to compare the declared `"vertexStride"` against the *intended* XNA-clean size of
each vertex type (16/20/24/32 — i.e. hardcoded constants, or a `sizeof` computed over a
`memcpy`-friendly bitwise-equivalent plain-data mirror struct with no `IVertexType` base),
not `sizeof()` of the actual (vtable-carrying) `Graphics::VertexPositionXxx` types
directly. Every other `SetData` call site that already constructs real, normal C++ vertex
objects field-by-field (not via `reinterpret_cast` on a raw byte blob) — e.g.
`HeightmapCollision`'s `Terrain.hpp`, `GeneratedGeometry`'s `Terrain.hpp`, this sample's
own new `CylinderModel.hpp` — is entirely unaffected by this bug, since normal C++
member-access always resolves through the real (inflated) layout correctly; the bug is
specific to `ModelTypeReader::Read()`'s stride-comparison-then-`reinterpret_cast` pattern.

**Workaround used in InverseKinematics (#057):** `samples/InverseKinematics/src/
CylinderModel.hpp` (NOXNA) reads `cylinder_verts.bin`/`cylinder_idx.bin` (produced once,
offline, by the *unchanged* `tools/obj2model.py`) directly and constructs real,
normally-initialized C++ `VertexPositionNormalTexture` objects (field-by-field, not a
`reinterpret_cast`), then uploads them through the same typed `VertexBuffer::SetData(const
VertexPositionNormalTexture*, count)` overload `ModelTypeReader` was trying (and failing)
to reach. Confirmed live: renders correctly (a lit, visibly shaded, 20-link cylinder chain
curling toward a target, exactly as expected) where the equivalent `Content.Load<Model>`
path rendered nothing at any scale/distance tried. See `samples/InverseKinematics/
missing.md` for the full write-up including every isolation step performed (sphere vs.
triangle vs. cylinder, scale sweeps, camera-distance sweeps, cull-mode sweeps, lighting
on/off).

**Blocked samples:** every `Content.Load<Model>`-based sample in this repo is *affected*
(their vertex data is silently corrupted), though most still render *something*
recognizable enough that this wasn't caught before (a corrupted-but-similar-magnitude
reinterpretation of a smoothly-varying, roughly-symmetric mesh like a sphere can still
look "sphere-ish", just distorted) — this item doesn't retroactively mark any of them as
newly broken, it re-attributes *why* the already-known thin-line/invisibility symptom
happens. InverseKinematics (#057) worked around it completely via `CylinderModel.hpp`.

**Effort:** S (the fix itself, once someone opens `ModelTypeReader::Read()`, is a few
constants) — most of the actual work is verifying the fix against every already-shipped
Model-based sample's screenshots to confirm no regression, since this reader is shared by
all of them.

---

## Summary Table

| # | Feature | Repo | Effort | Samples blocked |
|---|---|---|---|---|
| 1 | XNB → open format pipeline | all | M/sample | all |
| 2 | SpriteFont loading | cna | L | many | ✅ done |
| 3 | EasyGL: DiffuseColor ignored for VertexPositionColor | cna | S | Primitives3D | ✅ done |
| 4 | EasyGL: Wireframe mode (OpenGL ES has no glPolygonMode) | cna | M | Primitives3D | ✅ done |
| 5 | VertexPositionNormal + lit shader | cna | L | LensFlare, Graphics3D, PickingSample, TrianglePicking, HeightmapCollision, CustomModelClass, InverseKinematics, ChaseCamera, MarbleMaze | ✅ done for Model-based samples (2026-07-06); Primitives3D still needs a texture-less variant or port workaround |
| 6 | Model asset conversion, static geometry (.x/.fbx → .model.json) | tools | M/model | many | CNA itself works |
| 7 | Audio playback | cna | — | — | ✅ done (SDL3_mixer) |
| 8 | SpriteBatch.DrawString / SpriteFont | cna | M | most | ✅ done |
| 9 | Viewport.AspectRatio | cna | S | 0 (workaround) |
| 10 | GamePadButtons direct access | cna | — | 0 (workaround) |
| 11 | Shader conversion (HLSL .fx → GLSL .shader.json) | tools | M/shader | many Phase 3+ | CNA itself works |
| 12 | RenderTarget2D | cna | — | — | ✅ done |
| 13 | Skeletal animation playback (AnimationClip/Keyframe/AnimationPlayer) | cna | L/XL | SkinningSample, SkinnedModelExtensions, CPUSkinning, CustomModelAnimation | not started |
| 14 | TextureCube content loading (`Content.Load<TextureCube>`) | cna | S | RimLighting | not started |
| 15 | Accelerometer/sensor platform reality (documentation correction, not a gap) | docs | — | AccelerometerSample, TiltPerspective (scope decision, not blocked); Orientation (miscategorized, likely portable); Geolocation (still genuinely blocked) | ✅ no CNA change needed |
| 16 | Microphone capture | cna | M | MicrophoneEcho | ✅ done (merged 2026-07-04) |
| 17 | Multiplayer networking (NetworkSession-alike) | cna | L/XL | ClientServerSample, NetworkPrediction, PeerToPeer (NetRumble still needs item 11) | ✅ done (merged 2026-07-04) |
| 18 | Content-pipeline processor extensibility | tools | L | CustomModelEffect | not started |
| 19 | GamerServicesDispatcher::Update() no-op hangs NetworkSession::Create/Find/Join | cna | M | ClientServerSample, NetworkPrediction, PeerToPeer, NetRumble | ✅ done (fixed 2026-07-06) |
| 20 | NetworkGamer.IsHost/Id hardcoded stubs | cna | M | ClientServerSample, NetworkPrediction, PeerToPeer, NetRumble | ✅ done (fixed 2026-07-06; scoped remote-host-IsHost limitation remains, see item) |
| 21 | Initial GamerJoined event queued, not synchronous | cna + sharp-runtime | S | ClientServerSample, NetworkPrediction, PeerToPeer | ✅ done (fixed 2026-07-06 via sharp-runtime EventHandler<T>::SetReplayHook(), user-approved) |
| 22 | EasyGL: BlendState.ColorWriteChannels ignored (no glColorMask) | cna | S | LensFlare | not started |
| 23 | Game::DoInitialize() wires ComponentAdded after calling Initialize() | cna | S | Graphics3D, PickingSample (workaround applied); TrianglePicking confirmed NOT affected (constructor-time add) | not started |
| 24 | GraphicsDevice::Clear(Color) never clears depth buffer | cna | S | all 3D samples (latent, not blocking) | not started |
| 25 | VertexBuffer/IndexBuffer have no GetData() (no GPU buffer readback) | cna | S/M | none outright (tool-level workaround used by TrianglePicking) | not started |
| 26 | ModelTypeReader vertex-stride/IVertexType-vtable size mismatch corrupts all stride-32 .model.json vertex data (likely true cause of the "near-plane-clipping" bug family) | cna | S | every Content.Load<Model> sample (InverseKinematics worked around via CylinderModel.hpp) | not started |
